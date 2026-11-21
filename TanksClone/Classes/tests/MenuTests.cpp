#include "tests/MenuTests.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <string_view>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ObjectCast.h"
#include "services/InputManager.h"
#include "services/UiStyle.h"
#include "game/LevelLoader.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "model/MathTypes.h"
#include "scenes/EditorScene.h"
#include "game/ResourceManifest.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SettingsScene.h"
#include "scenes/UpgradesScene.h"
#include "services/ResourceManager.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "ui/CocosConvert.h"
#include "ui/ScreenFit.h"
#include "ui/Widgets.h"

namespace tanks
{
namespace tests
{

namespace
{
	struct SoundLoadState
	{
		bool bAnswered = false;
		SoundHandle Handle;
	};
	SoundLoadState GSoundLoad;

	typedef std::chrono::steady_clock Clock;

	// How long (real time) a step may wait for the screen it expects.
	constexpr float TimeoutSeconds = 15.0f;
	constexpr int MaxFpsPresses = 12;
	// The frame rate is counted over real time, after the new screen has settled.
	constexpr float WarmUpSeconds = 0.7f;
	constexpr float MeasureSeconds = 2.0f;
	constexpr float CapTolerance = 0.15f;
	// Above 60 the screen path may still limit the rate (a laptop's GPU copying frames to the display),
	// so a high limit only has to beat the menu's 60 clearly and switch vertical sync off.
	constexpr float FastRateMinimum = MenuFps + 6.0f;
	constexpr float WindowSizeTolerance = 2.0f;

	float SecondsSince(const Clock::time_point& InStart)
	{
		return std::chrono::duration<float>(Clock::now() - InStart).count();
	}

	struct Step
	{
		std::string Name;
		std::function<void(TestContext&)> Run;
		std::function<bool()> IsDone;
	};

	struct MenuRun
	{
		std::vector<Step> Steps;
		std::vector<TestResult> Results;
		std::function<void(const std::vector<TestResult>&)> OnDone;
		cocos2d::EventListenerCustom* Listener = nullptr;
		size_t Index = 0;
		bool bActing = true;
		Clock::time_point StepStart;
		std::string Detail; // what the last check saw, shown when a step times out
		std::unique_ptr<TestContext> Context;
	};

	MenuRun GRun;

	std::string Text(const std::string& InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}

	template <typename T>
	T* RunningScene()
	{
		auto Running = view::cocos::SceneCast<T>(cocos2d::Director::getInstance()->getRunningScene());
		return Running && Running->isRunning() ? Running : nullptr;
	}

	view::cocos::widgets::Button* FindButton(cocos2d::Node* const InRoot, std::string_view InPrefix)
	{
		if ( !InRoot || !InRoot->isVisible() )
		{
			return nullptr;
		}
		auto CurrentButton = view::cocos::widgets::Button::FromNode(InRoot);
		if ( CurrentButton && CurrentButton->GetText().compare(0, InPrefix.size(), InPrefix) == 0 )
		{
			return CurrentButton;
		}
		for ( cocos2d::Node* Child : InRoot->getChildren() )
		{
			if ( auto Found = FindButton(Child, InPrefix) )
			{
				return Found;
			}
		}
		return nullptr;
	}

	view::cocos::widgets::Button* FindButton(std::string_view InPrefix)
	{
		return FindButton(cocos2d::Director::getInstance()->getRunningScene(), InPrefix);
	}

	// Presses the button whose caption starts with the string InId; a missing button is a failure.
	void Press(TestContext& InContext, const std::string& InId)
	{
		auto CurrentButton = FindButton(Text(InId));
		InContext.Expect(CurrentButton != nullptr, "button '" + Text(InId) + "' is on the screen");
		if ( CurrentButton )
		{
			CurrentButton->Activate();
		}
	}

	void PressKey(const cocos2d::EventKeyboard::KeyCode InKey)
	{
		cocos2d::EventKeyboard Pressed(InKey, true);
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Pressed);
		cocos2d::EventKeyboard Released(InKey, false);
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Released);
	}

	bool HasButton(const std::string& InId)
	{
		return FindButton(Text(InId)) != nullptr;
	}

	void CollectButtonRects(cocos2d::Node* const InRoot, std::vector<model::RectF>& OutRects)
	{
		if ( !InRoot || !InRoot->isVisible() )
		{
			return;
		}
		if ( auto CurrentButton = view::cocos::widgets::Button::FromNode(InRoot) )
		{
			const cocos2d::Size Size = CurrentButton->getContentSize();
			const cocos2d::Vec2 Corner = CurrentButton->convertToWorldSpace(cocos2d::Vec2(-Size.width / 2, -Size.height / 2));
			const cocos2d::Vec2 Opposite = CurrentButton->convertToWorldSpace(cocos2d::Vec2(Size.width / 2, Size.height / 2));
			OutRects.emplace_back(Corner.x, Corner.y, Opposite.x - Corner.x, Opposite.y - Corner.y);
		}
		for ( cocos2d::Node* Child : InRoot->getChildren() )
		{
			CollectButtonRects(Child, OutRects);
		}
	}

	// Every button of the running scene is on the screen and none covers another.
	void ExpectButtonsApart(TestContext& InContext)
	{
		std::vector<model::RectF> Rects;
		CollectButtonRects(cocos2d::Director::getInstance()->getRunningScene(), Rects);
		const auto Director = cocos2d::Director::getInstance();
		const cocos2d::Vec2 Origin = Director->getVisibleOrigin();
		const cocos2d::Size Visible = Director->getVisibleSize();
		const model::RectF Screen(Origin.x - 0.5f, Origin.y - 0.5f, Visible.width + 1.0f, Visible.height + 1.0f);
		InContext.Expect(!Rects.empty(), "the scene has buttons");
		for ( std::size_t First = 0; First < Rects.size(); ++First )
		{
			const model::RectF& Rect = Rects[First];
			InContext.Expect(Screen.Contains(Rect.Origin) && Screen.Contains(Rect.Origin + Rect.Size), "button " + std::to_string(First) + " is on the screen ("
				+ std::to_string(static_cast<int>(Rect.GetMinX())) + "," + std::to_string(static_cast<int>(Rect.GetMinY())) + " - "
				+ std::to_string(static_cast<int>(Rect.GetMaxX())) + "," + std::to_string(static_cast<int>(Rect.GetMaxY())) + " in "
				+ std::to_string(static_cast<int>(Screen.GetMaxX())) + "x" + std::to_string(static_cast<int>(Screen.GetMaxY())) + ")");
			for ( std::size_t Second = First + 1; Second < Rects.size(); ++Second )
			{
				InContext.Expect(!Rect.Intersects(Rects[Second]), "buttons " + std::to_string(First) + " and " + std::to_string(Second) + " do not overlap");
			}
		}
	}

	view::cocos::EditorScene* Editor()
	{
		return RunningScene<view::cocos::EditorScene>();
	}

	// The editor shows tab InTab with all of its entries, and its first entry is selected.
	bool EditorShowsTab(const std::size_t InTab)
	{
		const auto CurrentEditor = Editor();
		if ( !CurrentEditor || CurrentEditor->GetActiveTab() != InTab )
		{
			return false;
		}
		const std::vector<std::size_t> Entries = CurrentEditor->GetPalette().GetEntriesOfTab(InTab);
		return !Entries.empty() && CurrentEditor->GetVisibleItemCount() == Entries.size()
			&& CurrentEditor->GetSelectedTile() == CurrentEditor->GetPalette().GetEntries()[Entries.front()].Tile;
	}

	PlayerProfile& Profile()
	{
		return GameContext::Get().GetManager<ProfileManager>().Get();
	}

	void Add(const std::string& InName, const std::function<void(TestContext&)>& InAction, const std::function<bool()>& InIsDone)
	{
		GRun.Steps.push_back({ InName, InAction, InIsDone });
	}

	void AddLanguageStep(const Language InExpected, const char* const InName)
	{
		Add(std::string("Settings: language switches to ") + InName,
			[](TestContext& InContext) { Press(InContext, "SETTINGS_LANGUAGE"); },
			[InExpected]() { return Profile().CurrentLanguage == InExpected && RunningScene<view::cocos::SettingsScene>() && HasButton("SETTINGS_LANGUAGE"); });
	}

	// Counts the frames drawn in MeasureSeconds of real time, after WarmUpSeconds, and checks the rate.
	// Called once per frame while the step waits.
	struct FrameMeter
	{
		Clock::time_point Start;
		bool bCounting = false;
		int Frames = 0;
	};

	// The most frames the screen path gives at all (unlimited, vertical sync off), measured by the first step.
	// A remote session or a virtual screen can hold it below 60; the limits are then checked against it.
	float GCapacity = 0.0f;

	// Counts frames until done; then checks the rate is at most InLimit (+ tolerance) and at least
	// InWanted, lowered to what the screen path can give.
	bool MeasureRate(FrameMeter& InMeter, const float InWanted, const float InLimit, const std::string& InWhat)
	{
		if ( !InMeter.bCounting )
		{
			if ( SecondsSince(GRun.StepStart) >= WarmUpSeconds )
			{
				InMeter.bCounting = true;
				InMeter.Start = Clock::now();
				InMeter.Frames = 0;
			}
			return false;
		}

		++InMeter.Frames;
		const float CurrentElapsed = SecondsSince(InMeter.Start);
		if ( CurrentElapsed < MeasureSeconds )
		{
			return false;
		}
		const float Rate = InMeter.Frames / CurrentElapsed;
		GRun.Context->Note(InWhat + ": " + std::to_string(static_cast<int>(Rate + 0.5f)) + " FPS, vertical sync interval "
			+ std::to_string(GetVerticalSyncInterval()));
		if ( GCapacity <= 0.0f )
		{
			GCapacity = Rate;
			if ( Rate < InWanted )
			{
				GRun.Context->Note("the screen path gives only " + std::to_string(static_cast<int>(Rate + 0.5f))
					+ " FPS here (a remote session or a virtual screen?): the limits are checked against that");
			}
			return true;
		}

		const float Minimum = std::min(InWanted, GCapacity) * (1 - CapTolerance);
		const float Maximum = InLimit > 0.0f ? InLimit * (1 + CapTolerance) : 1.0e9f;
		GRun.Context->Expect(Rate >= Minimum && Rate <= Maximum,
			InWhat + " between " + std::to_string(static_cast<int>(Minimum)) + " and " + std::to_string(static_cast<int>(std::min(Maximum, 99999.0f))) + " FPS");
		return true;
	}

	LevelData QuietLevel()
	{
		LevelData CurrentLevel = LevelLoader::CreateEmpty();
		CurrentLevel.Rows[GridRows - 1][GridColumns / 2] = tiles::PlayerSpawn;
		CurrentLevel.Rows[0][0] = tiles::EnemySpawn;
		CurrentLevel.TotalEnemies = 1;
		CurrentLevel.MaxAlive = 1;
		return CurrentLevel;
	}

	void AddLevelRateStep(const int InLimit, const float InWanted, const std::string& InName)
	{
		auto Meter = std::make_shared<FrameMeter>();
		Add(InName, [InLimit, Meter](TestContext&)
		{
			*Meter = FrameMeter();
			Profile().FpsLimit = InLimit;
			cocos2d::Director::getInstance()->replaceScene(view::cocos::GameScene::CreateForTest(QuietLevel(), false));
		}, [Meter, InLimit, InWanted]()
		{
			if ( !RunningScene<view::cocos::GameScene>() || !MeasureRate(*Meter, InWanted, static_cast<float>(InLimit), "a level") )
			{
				return false;
			}
			const int Vsync = GetVerticalSyncInterval();
			if ( Vsync >= 0 )
			{
				const bool bWanted = Profile().bVerticalSync;
				GRun.Context->Expect((Vsync != 0) == bWanted, std::string("vertical sync is ") + (bWanted ? "on" : "off") + " as set");
			}
			return true;
		});
	}

	void AddFrameRateSteps()
	{
		AddLevelRateStep(0, FastRateMinimum, "Frame rate: an unlimited level runs as fast as the screen path allows, vertical sync off");
		AddLevelRateStep(30, 30.0f, "Frame rate: a level keeps the 30 FPS limit");
		AddLevelRateStep(120, FastRateMinimum, "Frame rate: a level with the 120 FPS limit runs faster than the menu's 60, vertical sync off");

		auto Meter = std::make_shared<FrameMeter>();
		Add("Frame rate: the menu runs at 60 whatever the limit", [Meter](TestContext&)
		{
			*Meter = FrameMeter();
			cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene());
		}, [Meter]()
		{
			return RunningScene<view::cocos::MenuScene>() && MeasureRate(*Meter, static_cast<float>(MenuFps), static_cast<float>(MenuFps), "the menu");
		});
	}

	bool WindowIs(const PixelSize& InSize)
	{
		const cocos2d::Size CurrentFrame = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
		const PixelSize Client = GetWindowClientSize();
		GRun.Detail = "game frame " + std::to_string(static_cast<int>(CurrentFrame.width)) + "x" + std::to_string(static_cast<int>(CurrentFrame.height))
			+ ", window " + std::to_string(Client.Width) + "x" + std::to_string(Client.Height)
			+ ", expected " + std::to_string(InSize.Width) + "x" + std::to_string(InSize.Height);
		return std::fabs(CurrentFrame.width - InSize.Width) <= WindowSizeTolerance && std::fabs(CurrentFrame.height - InSize.Height) <= WindowSizeTolerance;
	}

	void AddWindowSizeStep(const int InScale)
	{
		Add("Settings: the window size " + std::to_string(InScale) + "% is applied at once", [](TestContext& InContext) { Press(InContext, "SETTINGS_WINDOW_SIZE"); },
			[InScale]() { return Profile().WindowScale == InScale && WindowIs(GetExpectedWindowedSize(InScale)) && HasButton("SETTINGS_WINDOW_SIZE"); });
	}

	void AddDisplaySettingsSteps()
	{
		Add("Settings: the effects volume goes down in steps and wraps round", [](TestContext& InContext)
		{
			const int Step = GameContext::Get().GetManager<UiStyle>().GetMenu().Settings.VolumeStep;
			Press(InContext, "SETTINGS_EFFECTS_VOLUME");
			InContext.ExpectEqual(Profile().EffectsVolume, 100 - Step, "one press");
			InContext.ExpectNear(GameContext::Get().GetManager<AudioManager>().GetVolume(), (100 - Step) / 100.0f, 0.001f, "the volume is applied");
			auto Volume = FindButton(Text("SETTINGS_EFFECTS_VOLUME") + ": " + std::to_string(Profile().EffectsVolume) + "%");
			InContext.Expect(Volume != nullptr, "the volume button is found");
			if ( Volume )
			{
				Volume->Adjust(1);
				Volume->Adjust(1);
				InContext.ExpectEqual(Profile().EffectsVolume, 100, "Right steps up and stops at the top");
				Volume->Adjust(-1);
				InContext.ExpectEqual(Profile().EffectsVolume, 100 - Step, "Left steps down");
			}
			for ( int Index = 0; Index < 100 / Step; ++Index )
			{
				Press(InContext, "SETTINGS_EFFECTS_VOLUME");
			}
		}, []() { return Profile().EffectsVolume == 100 && HasButton("SETTINGS_EFFECTS_VOLUME"); });

		Add("Settings: vertical sync switches on at once", [](TestContext& InContext)
		{
			Press(InContext, "SETTINGS_VSYNC");
			InContext.Expect(Profile().bVerticalSync && IsVerticalSyncEnabled(), "on in the profile and in the game");
			InContext.Expect(GetVerticalSyncInterval() != 0, "the driver reports it on");
			Press(InContext, "SETTINGS_VSYNC");
		}, []() { return !Profile().bVerticalSync && GetVerticalSyncInterval() <= 0 && HasButton("SETTINGS_VSYNC"); });

		Add("Settings: borderless fullscreen covers the screen", [](TestContext& InContext) { Press(InContext, "SETTINGS_WINDOW_MODE"); },
			[]() { return Profile().CurrentWindowMode == WindowMode::Borderless && WindowIs(GetScreenSize()) && HasButton("SETTINGS_WINDOW_MODE"); });
		Add("Screen: the design area stays centred where the window's aspect differs", [](TestContext& InContext)
		{
			auto Director = cocos2d::Director::getInstance();
			cocos2d::Scene* const Running = Director->getRunningScene();
			const cocos2d::Vec2 Offset = view::cocos::ToCocos(view::cocos::GetScreenLayout().GetDesignOffset());
			InContext.Expect(Offset.length() > 1.0f, "the screen's aspect needs an offset");
			InContext.Expect(Running->getPosition().distance(Offset) < 0.5f, "the scene is shifted by it");
		}, []() { return true; });
		Add("Settings: back to a window of the chosen size", [](TestContext& InContext) { Press(InContext, "SETTINGS_WINDOW_MODE"); },
			[]() { return Profile().CurrentWindowMode == WindowMode::Windowed && WindowIs(GetExpectedWindowedSize(Profile().WindowScale)) && HasButton("SETTINGS_WINDOW_MODE"); });
		Add("Settings: the window has the design proportions again, not the screen's", [](TestContext& InContext)
		{
			const PixelSize Window = GetExpectedWindowedSize(Profile().WindowScale);
			InContext.ExpectNear(static_cast<float>(Window.Width) / Window.Height, DesignWidth / DesignHeight, 0.01f, "windowed aspect");
		}, []() { return true; });
		AddWindowSizeStep(150);
		AddWindowSizeStep(200);
		AddWindowSizeStep(0);
		AddWindowSizeStep(100);

		Add("Settings: textures switch to SD and back to HD at once", [](TestContext& InContext)
		{
			const bool bWasHd = Profile().bHdTextures;
			Press(InContext, "SETTINGS_TEXTURES");
			InContext.Expect(Profile().bHdTextures != bWasHd && IsHdTextures() != bWasHd, "one press switches the set");
			InContext.ExpectNear(cocos2d::Director::getInstance()->getContentScaleFactor(),
				IsHdTextures() ? GetTextureQualityConfig().HdScale : 1.0f, 0.001f, "the content scale follows");
			Press(InContext, "SETTINGS_TEXTURES");
		}, []()
		{
			const std::string Atlas = "atlas.common";
			const std::string AtlasFile = "textures/common.plist";
			GRun.Detail = "loaded from " + GameContext::Get().GetManager<ResourceManager>().GetAtlasLoadedPath(Atlas);
			return Profile().bHdTextures == IsHdTextures()
				&& GameContext::Get().GetManager<ResourceManager>().GetAtlasLoadedPath(Atlas) == MapAtlasPath(AtlasFile, IsHdTextures(), GetTextureQualityConfig())
				&& HasButton("SETTINGS_TEXTURES");
		});

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		auto Wanted = std::make_shared<PixelPoint>(PixelPoint{ 0, 0 });
		Add("Settings: the window remembers where the player left it", [Wanted](TestContext& InContext)
		{
			GameContext::Get().GetManager<ProfileManager>().RememberWindowMoves();
			const PixelPoint Start = GetWindowPosition();
			MoveWindowTo({ Start.X + 37, Start.Y + 23 });
			*Wanted = GetWindowPosition();
			// What Windows sends when the player lets go of the dragged window.
			auto View = CheckedCast<cocos2d::GLViewImpl>(cocos2d::Director::getInstance()->getOpenGLView());
			SendMessage(View->getWin32Window(), WM_EXITSIZEMOVE, 0, 0);
			InContext.Expect(Profile().WindowPosition.has_value(), "the position is in the profile");
		}, [Wanted]()
		{
			const auto& Saved = Profile().WindowPosition;
			GRun.Detail = Saved ? "saved " + std::to_string(Saved->X) + "," + std::to_string(Saved->Y) : "nothing saved";
			return Saved && Saved->X == Wanted->X && Saved->Y == Wanted->Y;
		});
		Add("Settings: through borderless and back the window returns to that place", [](TestContext& InContext)
		{
			Press(InContext, "SETTINGS_WINDOW_MODE");
			InContext.Expect(Profile().CurrentWindowMode == WindowMode::Borderless, "borderless first");
			Press(InContext, "SETTINGS_WINDOW_MODE");
		}, [Wanted]()
		{
			const PixelPoint Now = GetWindowPosition();
			GRun.Detail = "window at " + std::to_string(Now.X) + "," + std::to_string(Now.Y) + ", expected " + std::to_string(Wanted->X) + "," + std::to_string(Wanted->Y);
			return Profile().CurrentWindowMode == WindowMode::Windowed && Now.X == Wanted->X && Now.Y == Wanted->Y && HasButton("SETTINGS_WINDOW_MODE");
		});
#endif

		Add("Settings: pixels can be smoothed and made sharp again", [](TestContext& InContext)
		{
			Press(InContext, "SETTINGS_PIXELS");
			InContext.Expect(Profile().bSmoothPixels && GameContext::Get().GetManager<ResourceManager>().IsSmoothPixels(), "smoothed at once");
			Press(InContext, "SETTINGS_PIXELS");
		}, []() { return !Profile().bSmoothPixels && !GameContext::Get().GetManager<ResourceManager>().IsSmoothPixels() && HasButton("SETTINGS_PIXELS"); });
	}

	float ButtonHeightOf(const std::string& InId)
	{
		auto CurrentButton = FindButton(Text(InId));
		return CurrentButton ? CurrentButton->getContentSize().height : 0.0f;
	}

	void AddTextSizeStep(const model::TextSize InExpected, const std::string& InName)
	{
		auto HeightBefore = std::make_shared<float>(0.0f);
		Add("Settings: text size " + InName + ", the buttons follow and stay apart", [HeightBefore](TestContext& InContext)
		{
			*HeightBefore = ButtonHeightOf("SETTINGS_SOUND");
			Press(InContext, "SETTINGS_TEXT_SIZE");
		}, [InExpected]()
		{
			const float Height = ButtonHeightOf("SETTINGS_SOUND");
			const UiStyle& Style = GameContext::Get().GetManager<UiStyle>();
			const float Expected = Style.GetMenu().Settings.ButtonHeight * Style.GetWidgets().TextScales.Get(InExpected);
			GRun.Detail = "button height " + std::to_string(Height) + ", expected " + std::to_string(Expected);
			if ( Profile().CurrentTextSize != InExpected || !RunningScene<view::cocos::SettingsScene>() || std::fabs(Height - Expected) > 0.5f )
			{
				return false;
			}
			ExpectButtonsApart(*GRun.Context);
			return true;
		});
	}

	void AddTextSizeSteps()
	{
		AddTextSizeStep(model::TextSize::Medium, "Medium");
		AddTextSizeStep(model::TextSize::Large, "Large");
		AddTextSizeStep(model::TextSize::Small, "Small");
	}

	void BuildSteps()
	{
		auto OnMenu = []() { return RunningScene<view::cocos::MenuScene>() != nullptr; };
		auto OnGame = []() { return RunningScene<view::cocos::GameScene>() != nullptr; };

		Add("Menu: the main menu opens", [](TestContext&)
		{
			cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene());
		}, OnMenu);

		Add("Settings: opens from the menu", [](TestContext& InContext) { Press(InContext, "MENU_SETTINGS"); },
			[]() { return RunningScene<view::cocos::SettingsScene>() != nullptr; });
		AddLanguageStep(Language::Ukrainian, "Ukrainian");
		AddLanguageStep(Language::Russian, "Russian");
		AddLanguageStep(Language::English, "English");

		auto SoundBefore = std::make_shared<bool>(false);
		Add("Settings: sound turns on and off", [SoundBefore](TestContext& InContext)
		{
			*SoundBefore = Profile().bSoundEnabled;
			Press(InContext, "SETTINGS_SOUND");
			InContext.Expect(Profile().bSoundEnabled != *SoundBefore, "the first press switches the sound");
			Press(InContext, "SETTINGS_SOUND");
		}, [SoundBefore]() { return Profile().bSoundEnabled == *SoundBefore && HasButton("SETTINGS_SOUND"); });

		auto FpsBefore = std::make_shared<int>(0);
		Add("Settings: the FPS limit goes through its options and back", [FpsBefore](TestContext& InContext)
		{
			*FpsBefore = Profile().FpsLimit;
			bool bChanged = false;
			for ( int Index = 0; Index < MaxFpsPresses; ++Index )
			{
				Press(InContext, "SETTINGS_FPS");
				bChanged = bChanged || Profile().FpsLimit != *FpsBefore;
				if ( Profile().FpsLimit == *FpsBefore )
				{
					break;
				}
			}
			InContext.Expect(bChanged, "a press changes the limit");
			InContext.Expect(Profile().FpsLimit == *FpsBefore, "the options come back to the first one");
		}, [FpsBefore]() { return Profile().FpsLimit == *FpsBefore && HasButton("SETTINGS_FPS"); });

		AddDisplaySettingsSteps();
		AddTextSizeSteps();

		Add("Settings: back to the menu", [](TestContext& InContext) { Press(InContext, "MENU_BACK"); }, OnMenu);
		Add("Settings: open again", [](TestContext& InContext) { Press(InContext, "MENU_SETTINGS"); },
			[]() { return RunningScene<view::cocos::SettingsScene>() != nullptr; });
		Add("Settings: Esc goes back to the menu", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE); }, OnMenu);

		Add("Upgrades: open from the menu", [](TestContext& InContext)
		{
			Profile().Coins = 0;
			Press(InContext, "MENU_UPGRADES");
		}, []() { return RunningScene<view::cocos::UpgradesScene>() != nullptr; });

		Add("Upgrades: nothing can be bought without coins", [](TestContext& InContext)
		{
			auto CurrentBuy = FindButton(Text("UPGRADE_BUY"));
			InContext.Expect(CurrentBuy != nullptr, "a buy button is shown");
			if ( CurrentBuy )
			{
				InContext.Expect(!CurrentBuy->IsEnabled(), "it is disabled");
				CurrentBuy->Activate();
			}
			InContext.Expect(Profile().Upgrades.empty(), "nothing was bought");
		}, []() { return true; });

		Add("Upgrades: back, get coins, come back", [](TestContext& InContext) { Press(InContext, "MENU_BACK"); }, OnMenu);
		Add("Upgrades: open again with coins", [](TestContext& InContext)
		{
			Profile().Coins = 1000;
			Press(InContext, "MENU_UPGRADES");
		}, []() { return RunningScene<view::cocos::UpgradesScene>() != nullptr; });

		Add("Upgrades: a purchase spends coins and raises the level", [](TestContext& InContext)
		{
			const auto& First = GameContext::Get().GetManager<UpgradeManager>().GetUpgrades().front();
			Press(InContext, "UPGRADE_BUY");
			InContext.ExpectEqual(GameContext::Get().GetManager<UpgradeManager>().GetLevel(First.Id), 1, First.Id + " level");
			InContext.ExpectEqual(Profile().Coins, 1000 - First.BaseCost, "coins left");
		}, []() { return RunningScene<view::cocos::UpgradesScene>() && HasButton("MENU_BACK"); });

		Add("Upgrades: the phone's Back key goes back to the menu", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_BACK); }, OnMenu);

		Add("Audio: a sound loads in the background and unloads with its handle", [](TestContext& InContext)
		{
			GSoundLoad.bAnswered = false;
			GSoundLoad.Handle.Reset();
			GameContext::Get().GetManager<ResourceManager>().AcquireSoundAsync("sfx.pause", [](SoundHandle InHandle)
			{
				GSoundLoad.Handle = std::move(InHandle);
				GSoundLoad.bAnswered = true;
			});
			InContext.Expect(!GSoundLoad.bAnswered || !GSoundLoad.Handle, "the answer comes later");
		}, []() { return GSoundLoad.bAnswered; });
		Add("Audio: the loaded sound is held by the handle", [](TestContext& InContext)
		{
			const bool bHeld = GameContext::Get().GetManager<ResourceManager>().IsSoundLoaded("sfx.pause");
			InContext.Expect(GSoundLoad.Handle ? bHeld : !bHeld, GSoundLoad.Handle ? "loaded while the handle lives" : "no audio device: the load failed cleanly");
			GSoundLoad.Handle.Reset();
			InContext.Expect(!GameContext::Get().GetManager<ResourceManager>().IsSoundLoaded("sfx.pause"), "unloaded after the handle is released");
		}, []() { return true; });

		Add("Game: Continue starts a level", [](TestContext& InContext) { Press(InContext, "MENU_CONTINUE"); }, OnGame);
		Add("Game: the loading screen left the level's tileset loaded", [](TestContext& InContext)
		{
			InContext.Expect(GameContext::Get().GetManager<ResourceManager>().IsAtlasLoaded(ResourceManifest().GetTilesetAtlas("")), "the tileset atlas is held by the level");
		}, []() { return true; });
		Add("Game: Esc pauses the level", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE); },
			[]() { return cocos2d::Director::getInstance()->isPaused() && HasButton("PAUSE_RESUME"); });
		auto DirectionBefore = std::make_shared<cocos2d::Vec2>();
		Add("Game: while paused, keys do not reach the tank", [DirectionBefore](TestContext& InContext)
		{
			auto Player = GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName);
			InContext.Expect(Player != nullptr, "the player's tank is there");
			*DirectionBefore = Player ? Player->GetDirection() : cocos2d::Vec2::ZERO;
			InContext.Expect(GameContext::Get().GetManager<InputManager>().GetActiveContext() == model::InputContext::Menu, "the menu context is on top of gameplay");
			const bool bTurnRight = DirectionBefore->x <= 0.5f;
			PressKey(bTurnRight ? cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW : cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW);
		}, []() { return HasButton("PAUSE_OPTIONS"); });
		Add("Game: Options open over the paused level", [](TestContext& InContext) { Press(InContext, "PAUSE_OPTIONS"); },
			[]() { return cocos2d::Director::getInstance()->isPaused() && HasButton("SETTINGS_EFFECTS_VOLUME") && !HasButton("PAUSE_RESUME"); });
		Add("Game: settings work over the level, texture quality is greyed out", [](TestContext& InContext)
		{
			const int VolumeBefore = Profile().EffectsVolume;
			Press(InContext, "SETTINGS_EFFECTS_VOLUME");
			InContext.Expect(Profile().EffectsVolume != VolumeBefore, "the volume changes while paused");
			InContext.Expect(FindButton(Text("SETTINGS_EFFECTS_VOLUME") + ": " + std::to_string(Profile().EffectsVolume)) != nullptr, "its caption follows at once");
			const bool bSmoothBefore = Profile().bSmoothPixels;
			Press(InContext, "SETTINGS_PIXELS");
			InContext.Expect(Profile().bSmoothPixels != bSmoothBefore && GameContext::Get().GetManager<ResourceManager>().IsSmoothPixels() != bSmoothBefore, "pixels switch while paused");
			Press(InContext, "SETTINGS_PIXELS");
			while ( Profile().EffectsVolume != VolumeBefore )
			{
				Press(InContext, "SETTINGS_EFFECTS_VOLUME");
			}
			const bool bHdBefore = Profile().bHdTextures;
			auto Textures = FindButton(Text("SETTINGS_TEXTURES"));
			InContext.Expect(Textures && !Textures->IsEnabled(), "the texture quality button is disabled");
			Press(InContext, "SETTINGS_TEXTURES");
			InContext.Expect(Profile().bHdTextures == bHdBefore, "and pressing it changes nothing");
		}, []() { return true; });
		Add("Game: Esc closes the options back to the pause menu", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE); },
			[]() { return cocos2d::Director::getInstance()->isPaused() && HasButton("PAUSE_RESUME") && !HasButton("SETTINGS_EFFECTS_VOLUME"); });
		Add("Game: Resume continues it", [](TestContext& InContext) { Press(InContext, "PAUSE_RESUME"); },
			[]() { return !cocos2d::Director::getInstance()->isPaused() && !view::cocos::widgets::Overlay::IsShown(); });
		Add("Game: the key pressed during the pause did not turn the tank", [DirectionBefore](TestContext& InContext)
		{
			auto Player = GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName);
			InContext.Expect(Player && Player->GetDirection() == *DirectionBefore, "the tank faces where it did before the pause");
			InContext.Expect(GameContext::Get().GetManager<InputManager>().GetActiveContext() == model::InputContext::Gameplay, "gameplay input is back on top");
		}, []() { return true; });

		Add("Game: the result screen appears when the tank is destroyed", [](TestContext& InContext)
		{
			auto Player = GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName);
			InContext.Expect(Player != nullptr, "the player's tank is there");
			if ( Player )
			{
				Player->GetBuffs().Remove("shield");
				Player->RefreshStats();
				Player->ApplyDamage(10000, Team::Enemy);
			}
		}, []() { return view::cocos::widgets::Overlay::IsShown() && HasButton("RESULT_RETRY"); });
		Add("Game: the result screen leads back to the menu", [](TestContext& InContext) { Press(InContext, "PAUSE_MENU"); }, OnMenu);

		Add("Game: a level can be left from the pause menu", [](TestContext& InContext) { Press(InContext, "MENU_CONTINUE"); }, OnGame);
		Add("Game: the phone's Back key pauses", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_BACK); },
			[]() { return cocos2d::Director::getInstance()->isPaused() && HasButton("PAUSE_RESUME"); });
		Add("Game: and Back again resumes", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_BACK); },
			[]() { return !cocos2d::Director::getInstance()->isPaused() && !view::cocos::widgets::Overlay::IsShown(); });
		Add("Game: pause again", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE); },
			[]() { return cocos2d::Director::getInstance()->isPaused() && HasButton("PAUSE_MENU"); });
		Add("Game: Main menu from the pause", [](TestContext& InContext) { Press(InContext, "PAUSE_MENU"); },
			[]() { return RunningScene<view::cocos::MenuScene>() && !cocos2d::Director::getInstance()->isPaused(); });

		Add("Editor: opens from the menu", [](TestContext& InContext) { Press(InContext, "MENU_EDITOR"); },
			[]() { return RunningScene<view::cocos::EditorScene>() != nullptr; });
		Add("Editor: the palette and action panels do not overlap", [](TestContext& InContext) { ExpectButtonsApart(InContext); },
			[]() { return EditorShowsTab(0); });
		Add("Editor: the bonuses tab shows the bonuses", [](TestContext& InContext) { Press(InContext, "EDITOR_TAB_BONUSES"); },
			[]() { return EditorShowsTab(2) && Editor()->GetSelectedTile() == tiles::Ammo; });
		Add("Editor: a digit key picks an item of the tab", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_2); },
			[]() { return Editor() && Editor()->GetSelectedTile() == tiles::Repair; });
		Add("Editor: Tab moves to the next tab", [](TestContext& InContext)
		{
			PressKey(cocos2d::EventKeyboard::KeyCode::KEY_TAB);
			ExpectButtonsApart(InContext);
		}, []() { return EditorShowsTab(3) && Editor()->GetSelectedTile() == tiles::Empty; });
		Add("Editor: back to the walls tab", [](TestContext& InContext) { Press(InContext, "EDITOR_TAB_WALLS"); },
			[]() { return EditorShowsTab(0) && Editor()->GetSelectedTile() == tiles::Brick; });
		Add("Editor: saves the map as the custom level", [](TestContext& InContext) { Press(InContext, "EDITOR_SAVE"); },
			[]() { return LevelLoader::HasCustom(); });
		Add("Editor: back to the menu", [](TestContext& InContext) { Press(InContext, "MENU_BACK"); }, OnMenu);

		Add("Menu: the custom level starts", [](TestContext& InContext) { Press(InContext, "MENU_CUSTOM_LEVEL"); }, OnGame);
		Add("Menu: and can be left", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE); },
			[]() { return cocos2d::Director::getInstance()->isPaused() && HasButton("PAUSE_MENU"); });
		Add("Menu: back in the menu", [](TestContext& InContext) { Press(InContext, "PAUSE_MENU"); },
			[]() { return RunningScene<view::cocos::MenuScene>() && !cocos2d::Director::getInstance()->isPaused(); });

		AddFrameRateSteps();
	}

	void Finish()
	{
		cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(GRun.Listener);
		GRun.Listener = nullptr;
		auto CurrentOnDone = GRun.OnDone;
		CurrentOnDone(GRun.Results);
	}

	void Record(const bool bInTimedOut)
	{
		const Step& CurrentStep = GRun.Steps[GRun.Index];
		TestResult Result;
		Result.Name = CurrentStep.Name;
		Result.Failures = GRun.Context->GetFailures();
		Result.Notes = GRun.Context->GetNotes();
		if ( bInTimedOut )
		{
			Result.Failures.push_back("the expected screen did not appear in time");
			if ( !GRun.Detail.empty() )
			{
				Result.Notes.push_back(GRun.Detail);
			}
		}
		GRun.Detail.clear();
		Result.bPassed = Result.Failures.empty();
		GRun.Results.push_back(Result);
	}

	void Tick()
	{
		if ( GRun.Index >= GRun.Steps.size() )
		{
			Finish();
			return;
		}

		const Step& CurrentStep = GRun.Steps[GRun.Index];
		if ( GRun.bActing )
		{
			GRun.Context.reset(new TestContext(nullptr));
			CurrentStep.Run(*GRun.Context);
			GRun.bActing = false;
			GRun.StepStart = Clock::now();
			return;
		}

		const bool bDone = CurrentStep.IsDone();
		if ( bDone || SecondsSince(GRun.StepStart) > TimeoutSeconds )
		{
			Record(!bDone);
			++GRun.Index;
			GRun.bActing = true;
			if ( !bDone )
			{
				// The screens after a failed step are unknown: stop here rather than report a chain of failures.
				GRun.Index = GRun.Steps.size();
			}
		}
	}
}

void RunMenuTests(const std::function<void(const std::vector<TestResult>&)>& InOnDone)
{
	GRun = MenuRun();
	GCapacity = 0.0f;
	GRun.OnDone = InOnDone;
	BuildSteps();

	// Ticks after every drawn frame: the scheduler stops while the game is paused, this event does not.
	GRun.Listener = cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(
		cocos2d::Director::EVENT_AFTER_DRAW, [](cocos2d::EventCustom*) { Tick(); });
}

} // namespace tests
} // namespace tanks

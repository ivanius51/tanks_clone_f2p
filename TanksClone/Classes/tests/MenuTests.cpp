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
#include "model/CustomLibrary.h"
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

	cocos2d::Label* FindLabelWithText(cocos2d::Node* const InRoot, const std::string& InText)
	{
		if ( !InRoot )
		{
			return nullptr;
		}
		if ( auto* const CurrentLabel = dynamic_cast<cocos2d::Label*>(InRoot); CurrentLabel && CurrentLabel->getString() == InText )
		{
			return CurrentLabel;
		}
		for ( cocos2d::Node* Child : InRoot->getChildren() )
		{
			if ( auto* const Found = FindLabelWithText(Child, InText) )
			{
				return Found;
			}
		}
		return nullptr;
	}

	// The title is fully above the first button (it overlapped it after a text size or language change).
	void ExpectTitleAboveButtons(TestContext& InContext, const std::string& InTitle)
	{
		cocos2d::Label* const TitleLabel = FindLabelWithText(cocos2d::Director::getInstance()->getRunningScene(), InTitle);
		InContext.Expect(TitleLabel != nullptr, "the title '" + InTitle + "' is on the screen");
		std::vector<model::RectF> Rects;
		CollectButtonRects(cocos2d::Director::getInstance()->getRunningScene(), Rects);
		if ( !TitleLabel || Rects.empty() )
		{
			return;
		}
		const cocos2d::Size Size = TitleLabel->getContentSize() * TitleLabel->getScale();
		const cocos2d::Vec2 Bottom = TitleLabel->getParent()->convertToWorldSpace(TitleLabel->getPosition() - cocos2d::Vec2(0, Size.height / 2));
		float HighestTop = Rects.front().GetMaxY();
		for ( const model::RectF& Rect : Rects )
		{
			HighestTop = std::max(HighestTop, Rect.GetMaxY());
		}
		InContext.Expect(Bottom.y >= HighestTop - 0.5f, "the title bottom " + std::to_string(static_cast<int>(Bottom.y)) + " is above the first button top " + std::to_string(static_cast<int>(HighestTop)));
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

	void AddLanguageStep(const std::string& InExpected)
	{
		Add("Settings: language switches to " + InExpected,
			[](TestContext& InContext) { Press(InContext, "SETTINGS_LANGUAGE"); },
			[InExpected]()
			{
				if ( Profile().CurrentLanguage != InExpected || !RunningScene<view::cocos::SettingsScene>() || !HasButton("SETTINGS_LANGUAGE") )
				{
					return false;
				}
				ExpectTitleAboveButtons(*GRun.Context, Text("SETTINGS_TITLE"));
				return true;
			});
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
			const float Full = Style.GetMenu().Settings.ButtonHeight * Style.GetWidgets().TextScales.Get(InExpected);
			GRun.Detail = "button height " + std::to_string(Height) + ", full size " + std::to_string(Full);
			if ( Profile().CurrentTextSize != InExpected || !RunningScene<view::cocos::SettingsScene>()
				|| Height > Full + 0.5f || Height < Full * Style.GetMenu().Settings.MinFitFactor - 0.5f )
			{
				return false;
			}
			ExpectButtonsApart(*GRun.Context);
			ExpectTitleAboveButtons(*GRun.Context, Text("SETTINGS_TITLE"));
			return true;
		});
	}

	void AddTextSizeSteps()
	{
		AddTextSizeStep(model::TextSize::Medium, "Medium");
		AddTextSizeStep(model::TextSize::Large, "Large");
		AddTextSizeStep(model::TextSize::Small, "Small");
	}

	void DispatchTouch(const cocos2d::Vec2& InWorld, const cocos2d::EventTouch::EventCode InCode)
	{
		static cocos2d::Touch Finger;
		const cocos2d::Vec2 Ui = cocos2d::Director::getInstance()->convertToUI(InWorld);
		Finger.setTouchInfo(0, Ui.x, Ui.y);
		cocos2d::EventTouch Event;
		Event.setEventCode(InCode);
		Event.setTouches({ &Finger });
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Event);
	}

	void TapWorld(const cocos2d::Vec2& InWorld)
	{
		DispatchTouch(InWorld, cocos2d::EventTouch::EventCode::BEGAN);
		DispatchTouch(InWorld, cocos2d::EventTouch::EventCode::ENDED);
	}

	void TapNode(const cocos2d::Node* const InNode)
	{
		if ( Editor() && InNode )
		{
			TapWorld(Editor()->GetWorldCentre(InNode));
		}
	}

	void RightClickWorld(const cocos2d::Vec2& InWorld)
	{
		cocos2d::EventMouse Down(cocos2d::EventMouse::MouseEventType::MOUSE_DOWN);
		Down.setMouseButton(cocos2d::EventMouse::MouseButton::BUTTON_RIGHT);
		const cocos2d::Vec2 Ui = cocos2d::Director::getInstance()->convertToUI(InWorld);
		Down.setCursorPosition(Ui.x, Ui.y);
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Down);
	}

	void ScrollWorld(const cocos2d::Vec2& InWorld, const float InY)
	{
		cocos2d::EventMouse Wheel(cocos2d::EventMouse::MouseEventType::MOUSE_SCROLL);
		Wheel.setScrollData(0.0f, InY);
		const cocos2d::Vec2 Ui = cocos2d::Director::getInstance()->convertToUI(InWorld);
		Wheel.setCursorPosition(Ui.x, Ui.y);
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Wheel);
	}

	bool IsCellInView(const model::EditorCell& InCell)
	{
		const auto* const CurrentEditor = Editor();
		const model::Vec2f InView = CurrentEditor->GetViewport().ContentToView(CurrentEditor->GetViewport().CellCenter(InCell, CurrentEditor->GetTileSize()));
		return model::RectF(model::Vec2f(0.0f, 0.0f), CurrentEditor->GetLayout().MapView.Size).Contains(InView);
	}

	bool FindCellInView(const std::function<bool(char)>& InTile, model::EditorCell& OutCell)
	{
		const auto* const CurrentEditor = Editor();
		if ( !CurrentEditor )
		{
			return false;
		}
		const model::EditorDocument& Document = CurrentEditor->GetDocument();
		for ( int Row = static_cast<int>(Document.GetRows().size()) - 1; Row >= 0; --Row )
		{
			for ( int Column = 0; Column < Document.GetColumnCount(); ++Column )
			{
				const model::EditorCell Cell{ Row, Column };
				if ( InTile(Document.GetTile(Cell)) && IsCellInView(Cell) )
				{
					OutCell = Cell;
					return true;
				}
			}
		}
		return false;
	}

	struct EditorFlow
	{
		model::EditorCell First;
		model::EditorCell Second;
		int Count = 0;
		std::size_t CustomBefore = 0;
		std::size_t CampaignsBefore = 0;
		model::Vec2f ScrollBefore;
	};

	void AddEditorClickSteps()
	{
		auto Flow = std::make_shared<EditorFlow>();
		using EditorType = view::cocos::EditorScene;
		const auto Cond = [](const std::function<bool(const EditorType&)>& InCheck)
		{
			return [InCheck]() { return Editor() != nullptr && InCheck(*Editor()); };
		};

		Add("Editor click: a wide screen keeps every dock open", [](TestContext&) { Editor()->SetAutoCollapse(false); },
			Cond([](const EditorType& E) { return !E.IsDockCollapsed(0) && !E.IsDockCollapsed(1) && !E.IsDockCollapsed(2); }));

		for ( const int Dock : { EditorType::LeftDock, EditorType::RightDock, EditorType::BottomDock } )
		{
			Add("Editor click: a dock handle collapses its dock and stays reachable", [Dock](TestContext& InContext)
			{
				InContext.Expect(Editor()->GetDockHandle(Dock) != nullptr, "the handle exists");
				TapNode(Editor()->GetDockHandle(Dock));
			}, Cond([Dock](const EditorType& E) { return E.IsDockCollapsed(Dock) && E.AreDockHandlesReachable(); }));
			Add("Editor click: the handle opens the dock again", [Dock](TestContext&) { TapNode(Editor()->GetDockHandle(Dock)); },
				Cond([Dock](const EditorType& E) { return !E.IsDockCollapsed(Dock) && E.AreDockHandlesReachable(); }));
		}

		Add("Editor click: the Library tab lists the levels", [](TestContext&) { TapNode(Editor()->GetDockTab(EditorType::LeftDock, 0)); },
			Cond([](const EditorType& E) { return E.GetLeftTab() == 0 && E.GetLibraryRow(0) != nullptr; }));
		Add("Editor click: a campaign level opens from the Library", [](TestContext&) { TapNode(Editor()->GetLibraryRow(1)); },
			Cond([](const EditorType& E) { return E.HasOpenedLevel() && E.GetOpenedEntry().Source == model::LibrarySource::Campaign; }));
		Add("Editor click: the mouse wheel scrolls a big map", [Flow](TestContext& InContext)
		{
			InContext.Expect(Editor()->GetViewport().CanScrollX() || Editor()->GetViewport().CanScrollY(), "the opened level is bigger than the view");
			Flow->ScrollBefore = Editor()->GetMapScroll();
			ScrollWorld(Editor()->LayoutToWorld(Editor()->GetLayout().MapView.GetCenter()), -3.0f);
		}, Cond([Flow](const EditorType& E)
		{
			const model::Vec2f Now = E.GetMapScroll();
			return Now.X != Flow->ScrollBefore.X || Now.Y != Flow->ScrollBefore.Y || !(E.GetViewport().CanScrollX() || E.GetViewport().CanScrollY());
		}));
		Add("Editor click: the Custom list is one tap away", [](TestContext& InContext) { Press(InContext, "EDITOR_LIB_CUSTOM"); },
			Cond([](const EditorType& E) { return E.GetLibrarySource() == model::LibrarySource::Custom; }));
		Add("Editor click: New level starts an empty map", [Flow](TestContext& InContext)
		{
			Flow->CustomBefore = Editor()->GetCustomLibrary().GetLevelIds().size();
			Press(InContext, "EDITOR_LIB_NEW");
		}, Cond([](const EditorType& E) { return !E.HasOpenedLevel() && E.GetHistory().GetSize() == 0; }));
		Add("Editor click: Place paints the empty cell that was tapped", [Flow](TestContext& InContext)
		{
			Editor()->SelectTab(0);
			InContext.Expect(FindCellInView([](const char InTile) { return InTile == tiles::Empty; }, Flow->First), "an empty cell is in view");
			TapWorld(Editor()->CellToWorld(Flow->First));
		}, Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Brick; }));
		Add("Editor click: a right click opens the menu with the object commands", [Flow](TestContext&) { RightClickWorld(Editor()->CellToWorld(Flow->First)); },
			Cond([](const EditorType& E) { return E.IsMenuOpen() && E.GetMenuLine(model::EditorMenuAction::Remove) != nullptr; }));
		Add("Editor click: Remove in the menu clears the cell", [Flow](TestContext&) { TapNode(Editor()->GetMenuLine(model::EditorMenuAction::Remove)); },
			Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Empty; }));
		Add("Editor click: Undo in the History tab brings the wall back", [](TestContext& InContext) { Press(InContext, "EDITOR_UNDO"); },
			Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Brick; }));
		Add("Editor click: Redo removes it again", [](TestContext& InContext) { Press(InContext, "EDITOR_REDO"); },
			Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Empty; }));
		Add("Editor click: a History row jumps back to the wall", [](TestContext& InContext)
		{
			InContext.Expect(Editor()->GetHistoryRow(1) != nullptr, "the row of the wall exists");
			TapNode(Editor()->GetHistoryRow(1));
		}, Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Brick && E.GetHistory().GetStep() == 1; }));
		Add("Editor click: the first History row jumps to the start", [](TestContext&) { TapNode(Editor()->GetHistoryRow(0)); },
			Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Empty && E.GetHistory().GetStep() == 0; }));

		Add("Editor click: a second wall goes on another empty cell", [Flow](TestContext& InContext)
		{
			Editor()->SelectTab(0);
			Flow->Second = model::EditorCell{ Flow->First.Row, Flow->First.Column + 1 };
			InContext.Expect(Editor()->GetDocument().GetTile(Flow->Second) == tiles::Empty && IsCellInView(Flow->Second), "the cell next to the first one is empty and in view");
			TapWorld(Editor()->CellToWorld(Flow->Second));
		}, Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->Second) == tiles::Brick; }));
		Add("Editor click: a right click opens the tool menu", [Flow](TestContext&) { RightClickWorld(Editor()->CellToWorld(Flow->First)); },
			Cond([](const EditorType& E) { return E.IsMenuOpen() && E.GetMenuLine(model::EditorMenuAction::ToolEraser) != nullptr; }));
		Add("Editor click: the menu switches to the Eraser", [](TestContext&) { TapNode(Editor()->GetMenuLine(model::EditorMenuAction::ToolEraser)); },
			Cond([](const EditorType& E) { return E.GetTool() == model::EditorTool::Eraser; }));
		Add("Editor click: the Eraser removes a tapped object", [Flow](TestContext&)
		{
			TapWorld(Editor()->CellToWorld(Flow->Second));
		}, Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->Second) == tiles::Empty; }));
		Add("Editor click: Undo gives the wall back", [](TestContext& InContext) { Press(InContext, "EDITOR_UNDO"); },
			Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->Second) == tiles::Brick; }));

		Add("Editor click: a right click opens the menu again", [Flow](TestContext&) { RightClickWorld(Editor()->CellToWorld(Flow->Second)); },
			Cond([](const EditorType& E) { return E.IsMenuOpen() && E.GetMenuLine(model::EditorMenuAction::ToolSelect) != nullptr; }));
		Add("Editor click: the menu switches to Select", [](TestContext&) { TapNode(Editor()->GetMenuLine(model::EditorMenuAction::ToolSelect)); },
			Cond([](const EditorType& E) { return E.GetTool() == model::EditorTool::Select; }));
		Add("Editor click: Select jumps the palette to the item of the object", [Flow](TestContext& InContext)
		{
			for ( std::size_t Item = 0; Item < 64 && Editor()->GetSelectedTile() == tiles::Brick; ++Item )
			{
				Editor()->SelectItem(Item);
			}
			InContext.Expect(Editor()->GetSelectedTile() != tiles::Brick, "another palette item is selected first");
			TapWorld(Editor()->CellToWorld(Flow->Second));
		}, Cond([](const EditorType& E) { return E.GetSelectedTile() == tiles::Brick; }));

		Add("Editor click: a right click opens the menu to go back to Place", [Flow](TestContext&) { RightClickWorld(Editor()->CellToWorld(Flow->First)); },
			Cond([](const EditorType& E) { return E.IsMenuOpen() && E.GetMenuLine(model::EditorMenuAction::ToolPlace) != nullptr; }));
		Add("Editor click: the Place tool is on", [](TestContext&) { TapNode(Editor()->GetMenuLine(model::EditorMenuAction::ToolPlace)); },
			Cond([](const EditorType& E) { return E.GetTool() == model::EditorTool::Place; }));
		Add("Editor click: place a wall to move", [Flow](TestContext&)
		{
			Editor()->SelectTab(0);
			TapWorld(Editor()->CellToWorld(Flow->First));
		}, Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->First) == tiles::Brick; }));
		Add("Editor click: a right click on the wall offers Move", [Flow](TestContext&) { RightClickWorld(Editor()->CellToWorld(Flow->First)); },
			Cond([](const EditorType& E) { return E.IsMenuOpen() && E.GetMenuLine(model::EditorMenuAction::MoveObject) != nullptr; }));
		Add("Editor click: Move picks the wall up", [Flow](TestContext& InContext)
		{
			TapNode(Editor()->GetMenuLine(model::EditorMenuAction::MoveObject));
			model::EditorCell Other;
			InContext.Expect(FindCellInView([](const char InTile) { return InTile == tiles::Empty; }, Other) && !(Other == Flow->First), "another empty cell is in view");
			Flow->Second = Other;
		}, Cond([](const EditorType& E) { return E.GetTool() == model::EditorTool::Move; }));
		Add("Editor click: the next tap puts the wall down", [Flow](TestContext&) { TapWorld(Editor()->CellToWorld(Flow->Second)); },
			Cond([Flow](const EditorType& E) { return E.GetDocument().GetTile(Flow->Second) == tiles::Brick && E.GetDocument().GetTile(Flow->First) == tiles::Empty; }));

		Add("Editor click: the Level tab shows the enemy stepper", [](TestContext&) { TapNode(Editor()->GetDockTab(EditorType::LeftDock, 1)); },
			Cond([](const EditorType& E) { return E.GetLeftTab() == 1 && E.IsEnemyStepperInsideDock(); }));
		Add("Editor click: the stepper plus raises the enemy count", [Flow](TestContext& InContext)
		{
			Flow->Count = Editor()->GetEnemyCount();
			TapNode(Editor()->GetEnemyStepperButton(true));
			InContext.Expect(Editor()->GetEnemyCount() == Flow->Count + 1, "one more enemy");
		}, []() { return true; });
		Add("Editor click: the stepper minus lowers it back", [Flow](TestContext& InContext)
		{
			TapNode(Editor()->GetEnemyStepperButton(false));
			InContext.Expect(Editor()->GetEnemyCount() == Flow->Count, "the count is back");
		}, []() { return true; });

		Add("Editor click: Save keeps the new level as a custom copy", [](TestContext& InContext) { Press(InContext, "EDITOR_SAVE"); },
			Cond([Flow](const EditorType& E)
			{
				return E.HasOpenedLevel() && E.GetOpenedEntry().Source == model::LibrarySource::Custom && E.GetCustomLibrary().GetLevelIds().size() == Flow->CustomBefore + 1;
			}));

		Add("Editor click: the Campaigns tab opens", [](TestContext&) { TapNode(Editor()->GetDockTab(EditorType::LeftDock, 2)); },
			Cond([](const EditorType& E) { return E.GetLeftTab() == 2; }));
		Add("Editor click: New campaign adds one and selects it", [Flow](TestContext& InContext)
		{
			Flow->CampaignsBefore = Editor()->GetCustomLibrary().GetCampaigns().size();
			Press(InContext, "EDITOR_CAMP_NEW");
		}, Cond([Flow](const EditorType& E)
		{
			return E.GetCustomLibrary().GetCampaigns().size() == Flow->CampaignsBefore + 1 && !E.GetCampaignId().empty() && E.GetCampaignAddRow(0) != nullptr;
		}));
		Add("Editor click: tapping a level in the add list puts it into the campaign", [](TestContext&) { TapNode(Editor()->GetCampaignAddRow(0)); },
			Cond([](const EditorType& E)
			{
				const model::CampaignDef* const Campaign = E.GetCustomLibrary().FindCampaign(E.GetCampaignId());
				return Campaign && Campaign->LevelIds.size() == 1;
			}));
		Add("Editor click: the campaign is saved through the storage port", [Flow](TestContext& InContext)
		{
			model::CustomLibrary Stored;
			InContext.Expect(Stored.Load(GameContext::Get().GetStorage()), "the index loads");
			const model::CampaignDef* const Campaign = Stored.FindCampaign(Editor()->GetCampaignId());
			InContext.Expect(Campaign && Campaign->LevelIds.size() == 1, "the stored campaign has the level");
		}, []() { return true; });
		Add("Editor click: Delete campaign removes it again", [Flow](TestContext& InContext) { Press(InContext, "EDITOR_CAMP_DELETE"); },
			Cond([Flow](const EditorType& E) { return E.GetCustomLibrary().GetCampaigns().size() == Flow->CampaignsBefore; }));
	}

	void AddEditorSteps()
	{
		Add("Editor: the palette and action panels do not overlap", [](TestContext& InContext) { ExpectButtonsApart(InContext); },
			[]() { return EditorShowsTab(0); });
		Add("Editor: the bonuses tab shows the bonuses", [](TestContext& InContext) { InContext.Expect(Editor() != nullptr, "the editor runs"); Editor()->SelectTab(2); },
			[]() { return EditorShowsTab(2) && Editor()->GetSelectedTile() == tiles::Ammo; });
		Add("Editor: a digit key picks an item of the tab", [](TestContext&) { PressKey(cocos2d::EventKeyboard::KeyCode::KEY_2); },
			[]() { return Editor() && Editor()->GetSelectedTile() == tiles::Repair; });
		Add("Editor: Tab moves to the next tab", [](TestContext& InContext)
		{
			PressKey(cocos2d::EventKeyboard::KeyCode::KEY_TAB);
			ExpectButtonsApart(InContext);
		}, []() { return EditorShowsTab(3) && Editor()->GetSelectedTile() == tiles::Empty; });
		Add("Editor: back to the walls tab", [](TestContext& InContext) { InContext.Expect(Editor() != nullptr, "the editor runs"); Editor()->SelectTab(0); },
			[]() { return EditorShowsTab(0) && Editor()->GetSelectedTile() == tiles::Brick; });
		Add("Editor: saves the map as the custom level", [](TestContext& InContext) { Press(InContext, "EDITOR_SAVE"); },
			[]() { return LevelLoader::HasCustom(); });
		Add("Editor: the palette tiles sit inside the bottom dock", [](TestContext& InContext)
		{
			InContext.Expect(Editor() != nullptr && Editor()->AreTilesInsideDock(), "every tile is inside the dock");
		}, []() { return true; });
		Add("Editor: the enemy counter fits inside the left dock", [](TestContext& InContext)
		{
			InContext.Expect(Editor() != nullptr && Editor()->IsEnemyStepperInsideDock(), "the stepper is inside the dock");
		}, []() { return true; });
		Add("Editor: every campaign level opens in the editor", [](TestContext& InContext)
		{
			InContext.Expect(Editor() != nullptr && Editor()->CountLibraryLevelsThatFailToOpen(model::LibrarySource::Campaign) == 0, "no campaign level fails to open");
			InContext.Expect(Editor() != nullptr && Editor()->CountLibraryLevelsThatFailToOpen(model::LibrarySource::Test) == 0, "no test level fails to open");
		}, []() { return true; });
		Add("Editor: collapsed docks keep a reachable handle and open again", [](TestContext& InContext)
		{
			InContext.Expect(Editor() != nullptr, "the editor runs");
			if ( !Editor() )
			{
				return;
			}
			Editor()->SetAllDocksCollapsed(true);
			InContext.Expect(Editor()->AreDockHandlesReachable(), "every handle is visible inside its collapsed dock");
			Editor()->SetAllDocksCollapsed(false);
			InContext.Expect(Editor()->AreTilesInsideDock() && Editor()->AreDockHandlesReachable(), "the docks are back");
		}, []() { return true; });
	}

	void AddCampaignPlaySteps()
	{
		auto CampaignId = std::make_shared<std::string>();
		Add("Campaign: a custom campaign level starts and the profile stays untouched", [CampaignId](TestContext& InContext)
		{
			ports::IStorage& Storage = GameContext::Get().GetStorage();
			LevelData Source;
			InContext.Expect(LevelLoader::Load(LevelLoader::BuiltinId(0), Source), "a shipped level loads");
			model::CustomLibrary Library;
			Library.Load(Storage);
			const std::string LevelId = "campaign_test_level";
			Storage.WriteText(model::CustomLibrary::LevelFileName(LevelId), LevelLoader::ToJson(Source));
			Library.AddLevel(LevelId);
			*CampaignId = Library.AddCampaign("Test");
			Library.SetCampaignLevels(*CampaignId, { LevelId, LevelId });
			Library.Save(Storage);
			cocos2d::Scene* const Scene = view::cocos::GameScene::CreateCampaignScene(*CampaignId, 0);
			InContext.Expect(Scene != nullptr, "the campaign level loads from the storage");
			if ( Scene )
			{
				cocos2d::Director::getInstance()->replaceScene(Scene);
			}
		}, []() { return RunningScene<view::cocos::GameScene>() != nullptr; });
		Add("Campaign: the stored test campaign is removed and the menu opens", [CampaignId](TestContext&)
		{
			ports::IStorage& Storage = GameContext::Get().GetStorage();
			model::CustomLibrary Library;
			Library.Load(Storage);
			Library.RemoveCampaign(*CampaignId);
			Library.RemoveLevel("campaign_test_level");
			Library.Save(Storage);
			cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene());
		}, []() { return RunningScene<view::cocos::MenuScene>() != nullptr; });
	}

	void BuildSteps(const bool bInEditorOnly)
	{
		if ( bInEditorOnly )
		{
			Add("Editor: opens", [](TestContext&) { cocos2d::Director::getInstance()->replaceScene(view::cocos::EditorScene::CreateScene()); },
				[]() { return RunningScene<view::cocos::EditorScene>() != nullptr; });
			AddEditorClickSteps();
			Add("Editor: opens again", [](TestContext&) { cocos2d::Director::getInstance()->replaceScene(view::cocos::EditorScene::CreateScene()); },
				[]() { return RunningScene<view::cocos::EditorScene>() != nullptr; });
			AddEditorSteps();
			AddCampaignPlaySteps();
			return;
		}
		auto OnMenu = []() { return RunningScene<view::cocos::MenuScene>() != nullptr; };
		auto OnGame = []() { return RunningScene<view::cocos::GameScene>() != nullptr; };

		Add("Menu: the main menu opens", [](TestContext&)
		{
			cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene());
		}, OnMenu);

		Add("Settings: opens from the menu", [](TestContext& InContext) { Press(InContext, "MENU_SETTINGS"); },
			[]() { return RunningScene<view::cocos::SettingsScene>() != nullptr; });
		{
			const StringsManager& Strings = GameContext::Get().GetManager<StringsManager>();
			std::string Id = Strings.GetLanguageId();
			for ( size_t Count = 0; Count < Strings.GetLanguages().size(); ++Count )
			{
				Id = Strings.GetNextLanguageId(Id);
				AddLanguageStep(Id);
			}
		}

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
		AddEditorSteps();
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

void RunMenuTests(const std::function<void(const std::vector<TestResult>&)>& InOnDone, const bool bInEditorOnly)
{
	GRun = MenuRun();
	GCapacity = 0.0f;
	GRun.OnDone = InOnDone;
	BuildSteps(bInEditorOnly);

	// Ticks after every drawn frame: the scheduler stops while the game is paused, this event does not.
	GRun.Listener = cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(
		cocos2d::Director::EVENT_AFTER_DRAW, [](cocos2d::EventCustom*) { Tick(); });
}

} // namespace tests
} // namespace tanks

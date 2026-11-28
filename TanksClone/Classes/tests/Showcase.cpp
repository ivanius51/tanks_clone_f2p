#include "tests/Showcase.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <fstream>
#include <sstream>
#include <typeinfo>

#include "base/ccUtils.h"

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/GameData.h"
#include "core/ObjectCast.h"
#include "game/LevelLoader.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Tank.h"
#include "platform/cocos/CocosFrameCapture.h"
#include "scenes/EditorScene.h"
#include "scenes/GameScene.h"
#include "scenes/LoadingScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SettingsScene.h"
#include "scenes/UpgradesScene.h"
#include "scenes/WidgetEditorScene.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/UpgradeManager.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "tests/Galleries.h"
#include "tests/TestRunner.h"
#include "ui/ScreenFit.h"
#include "ui/Widgets.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <stdlib.h>
#endif

namespace game
{
namespace tests
{

namespace
{
	typedef std::chrono::steady_clock Clock;

	constexpr float ScreenSettleSeconds = 1.0f;
	constexpr float LoadingHoldSeconds = 2.0f;
	constexpr float LoadingShotSeconds = 1.2f;
	constexpr float LevelPlaySeconds = 4.0f;
	constexpr float ClipWarmUpSeconds = 2.0f;
	constexpr float ClipSeconds = 5.0f;
	constexpr int ClipFramesPerSecond = 30;
	constexpr float MinFrameSeconds = 0.001f;
	constexpr int ShowcaseCoins = 750;
	constexpr int BossLevelIndex = 5;
	constexpr int ClipLevelIndex = 2;
	constexpr float DropKillSeconds = 1.5f;
	constexpr float DropKillInterval = 0.8f;
	constexpr unsigned int DropKillRepeats = 6;
	constexpr float DropShotSeconds = 7.0f;
	constexpr int DropKillDamage = 100000;
	constexpr float LookGallerySeconds = 2.5f;
	constexpr std::string_view LookGalleryId = "tank_look";
	constexpr float StayOpenSeconds = 1.0e9f;
	constexpr float ResizeSettleSeconds = 0.5f;
	constexpr int MatrixHeights[]{ 720, 1080, 1440, 2160 };
	// Width over height as a ratio of whole numbers: 4:3, 16:9, 21:9.
	constexpr int MatrixAspects[][2]{ { 4, 3 }, { 16, 9 }, { 21, 9 } };
	constexpr int MatrixLevelIndex = 0;
	constexpr float BoundsTolerance = 1.0f;
	constexpr int MatrixFailureExitCode = 1;
	constexpr std::string_view MatrixReportName = "matrix_report.txt";

	// One thing to show: build the screen, wait, then take a picture (or record the clip).
	struct Shot
	{
		TFunction<void(), UiCallbackInlineSize> Show;
		float WaitSeconds;
		std::string FileName; // empty: the clip
		int CaptureWidth = 0; // 0: the window back buffer
		int CaptureHeight = 0;
	};

	struct ShowcaseRun
	{
		std::string Folder;
		TArray<Shot> Shots;
		size_t Index = 0;
		bool bShown = false;
		bool bCapturing = false;
		Clock::time_point ShownAt;
		Clock::time_point ClipStart;
		int ClipFrames = 0;
		int ClipCaptured = 0;
		bool bVirtualFrame = false;
		cocos2d::Size RealFrame;
		std::string Report;
		int Failures = 0;
		int CheckedLabels = 0;
		int CheckedSprites = 0;
		Clock::time_point RunStart;
	};

	ShowcaseRun GShow;

	float SecondsSince(const Clock::time_point& InStart)
	{
		return std::chrono::duration<float>(Clock::now() - InStart).count();
	}

	LevelData BuiltinLevel(const int InIndex)
	{
		LevelData CurrentLevel;
		LevelLoader::Load(LevelLoader::BuiltinId(InIndex), CurrentLevel);
		return CurrentLevel;
	}

	// The last level with only its last wave: the boss comes at once.
	LevelData BossLevel()
	{
		LevelData CurrentLevel = BuiltinLevel(BossLevelIndex);
		if ( CurrentLevel.Waves.Size() > 1 )
		{
			CurrentLevel.Waves.Erase(CurrentLevel.Waves.begin(), CurrentLevel.Waves.end() - 1);
			CurrentLevel.Waves.Front().Delay = 0.0f;
		}
		return CurrentLevel;
	}

	void PlayLevel(const LevelData& InLevel)
	{
		cocos2d::Director::getInstance()->replaceScene(view::cocos::GameScene::CreateForTest(InLevel));
	}

	// The boss level, then every enemy is destroyed so the boss's drop table (always drops) shows a pickup.
	void PlayDropLevel()
	{
		PlayLevel(BossLevel());
		cocos2d::Director::getInstance()->getScheduler()->schedule([](float)
		{
			TArray<Tank*> Enemies;
			GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&Enemies](Tank& InTank)
			{
				if ( InTank.GetTeam() == ETeam::Enemy && !InTank.IsDestroying() )
				{
					Enemies.PushBack(&InTank);
				}
			});
			for ( Tank* const Enemy : Enemies )
			{
				Enemy->ApplyDamage(DropKillDamage, ETeam::Player);
			}
		}, &GShow, DropKillInterval, DropKillRepeats, DropKillSeconds, false, "showcase_drop_kill");
	}

	bool HasArgument(const wchar_t* const InName)
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		for ( int Index = 1; __wargv && Index < __argc; ++Index )
		{
			if ( std::wstring(__wargv[Index]) == InName )
			{
				return true;
			}
		}
#endif
		return false;
	}

	// The value of --part=<name> (empty when absent): one showcase part, e.g. --part=editor.
	std::string ParseShowcasePart(std::string_view InCommandLine)
	{
		constexpr std::string_view Key = "--part=";
		const std::size_t Start = InCommandLine.find(Key);
		if ( Start == std::string_view::npos )
		{
			return {};
		}
		const std::size_t ValueStart = Start + Key.size();
		return std::string(InCommandLine.substr(ValueStart, InCommandLine.find(' ', ValueStart) - ValueStart));
	}

	// --art-style=<0..2> (pixel art, classic, 3D look); the profile default when absent.
	std::optional<int> ParseShowcaseArtStyle(std::string_view InCommandLine)
	{
		constexpr std::string_view Key = "--art-style=";
		const std::size_t Start = InCommandLine.find(Key);
		if ( Start == std::string_view::npos )
		{
			return std::nullopt;
		}
		return std::atoi(std::string(InCommandLine.substr(Start + Key.size(), 1)).c_str());
	}

	std::string LevelPartName(const int InIndex)
	{
		return cocos2d::StringUtils::format("level_%d", InIndex + 1);
	}

	void AddLevelPart(const int InIndex)
	{
		GShow.Shots.PushBack({ [InIndex]() { PlayLevel(BuiltinLevel(InIndex)); }, LevelPlaySeconds, cocos2d::StringUtils::format("level_%d.png", InIndex + 1) });
	}

	// Gives the layout a frame of this size without touching the window: the matrix is then drawn offscreen at any size.
	void SetVirtualFrame(const int InWidth, const int InHeight)
	{
		cocos2d::GLView* const View = cocos2d::Director::getInstance()->getOpenGLView();
		if ( !GShow.bVirtualFrame )
		{
			GShow.RealFrame = View->getFrameSize();
			GShow.bVirtualFrame = true;
		}
		// The base class only stores the size; the desktop override would resize the real window.
		View->cocos2d::GLView::setFrameSize(static_cast<float>(InWidth), static_cast<float>(InHeight));
		view::cocos::ApplyScreenFit();
	}

	void RestoreRealFrame()
	{
		if ( !GShow.bVirtualFrame )
		{
			return;
		}
		GShow.bVirtualFrame = false;
		cocos2d::Director::getInstance()->getOpenGLView()->cocos2d::GLView::setFrameSize(GShow.RealFrame.width, GShow.RealFrame.height);
		view::cocos::ApplyScreenFit();
	}

	const view::cocos::widgets::Button* FindOwnerButton(const cocos2d::Node* InNode)
	{
		for ( InNode = InNode->getParent(); InNode; InNode = InNode->getParent() )
		{
			if ( const auto* const Owner = view::cocos::widgets::Button::FromNode(InNode) )
			{
				return Owner;
			}
		}
		return nullptr;
	}

	cocos2d::Rect WorldBounds(const cocos2d::Node* const InNode, const cocos2d::Vec2& InLocalOrigin = cocos2d::Vec2::ZERO)
	{
		const cocos2d::Size& Size = InNode->getContentSize();
		return cocos2d::RectApplyAffineTransform(cocos2d::Rect(InLocalOrigin, Size), InNode->getNodeToWorldAffineTransform());
	}

	// A button draws its frame centered on its own origin (see Button::IsUnderCursor).
	cocos2d::Rect ButtonBounds(const view::cocos::widgets::Button* const InButton)
	{
		const cocos2d::Size& Size = InButton->getContentSize();
		return WorldBounds(InButton, cocos2d::Vec2(-Size.width / 2.0f, -Size.height / 2.0f));
	}

	bool Encloses(const cocos2d::Rect& InOuter, const cocos2d::Rect& InInner)
	{
		return InInner.getMinX() >= InOuter.getMinX() - BoundsTolerance && InInner.getMaxX() <= InOuter.getMaxX() + BoundsTolerance
			&& InInner.getMinY() >= InOuter.getMinY() - BoundsTolerance && InInner.getMaxY() <= InOuter.getMaxY() + BoundsTolerance;
	}

	void AddFailure(const std::string& InShot, const std::string& InText)
	{
		++GShow.Failures;
		GShow.Report += "  FAIL " + InShot + ": " + InText + "\n";
	}

	void CheckLabel(const std::string& InShot, const cocos2d::Label* const InLabel)
	{
		const view::cocos::widgets::Button* const Owner = FindOwnerButton(InLabel);
		if ( !Owner || InLabel->getString().empty() )
		{
			return;
		}
		++GShow.CheckedLabels;
		const cocos2d::Rect Text = WorldBounds(InLabel);
		const cocos2d::Rect Frame = ButtonBounds(Owner);
		if ( !Encloses(Frame, Text) )
		{
			AddFailure(InShot, cocos2d::StringUtils::format("label '%s' (%.0fx%.0f) overflows button '%s' (%.0fx%.0f)", InLabel->getString().c_str(),
				Text.size.width, Text.size.height, Owner->GetText().c_str(), Frame.size.width, Frame.size.height));
		}
	}

	void CheckSprite(const std::string& InShot, const cocos2d::Sprite* const InSprite)
	{
		++GShow.CheckedSprites;
		const cocos2d::Size& Size = InSprite->getContentSize();
		if ( !InSprite->getTexture() || Size.width <= 0.0f || Size.height <= 0.0f )
		{
			AddFailure(InShot, cocos2d::StringUtils::format("sprite tag %d has %s", InSprite->getTag(), InSprite->getTexture() ? "zero size" : "no texture"));
		}
	}

	// typeid matches the exact engine class, so no type is guessed and subclasses with their own drawing are left alone.
	void CheckTree(const std::string& InShot, const cocos2d::Node* const InNode)
	{
		if ( !InNode->isVisible() )
		{
			return;
		}
		if ( typeid(*InNode) == typeid(cocos2d::Label) )
		{
			CheckLabel(InShot, static_cast<const cocos2d::Label*>(InNode));
		}
		else if ( typeid(*InNode) == typeid(cocos2d::Sprite) )
		{
			CheckSprite(InShot, static_cast<const cocos2d::Sprite*>(InNode));
		}
		for ( const cocos2d::Node* const Child : InNode->getChildren() )
		{
			CheckTree(InShot, Child);
		}
	}

	bool SaveOffscreenShot(const Shot& InShot)
	{
		TArray<std::uint8_t> Pixels;
		int Width = 0;
		int Height = 0;
		if ( !view::cocos::CocosFrameCapture::CaptureScreen(cocos2d::Director::getInstance()->getRunningScene(), InShot.CaptureWidth, InShot.CaptureHeight, Pixels, Width, Height) )
		{
			return false;
		}
		cocos2d::Image* const Picture = new cocos2d::Image();
		const bool bSaved = Picture->initWithRawData(Pixels.GetData(), static_cast<ssize_t>(Pixels.Size()), Width, Height, 8)
			&& Picture->saveToFile(GShow.Folder + InShot.FileName, false);
		Picture->release();
		return bSaved;
	}

	// False when the offscreen texture could not be made: the shot then falls back to the window back buffer.
	bool CaptureMatrixShot(const Shot& InShot)
	{
		const Clock::time_point Start = Clock::now();
		const bool bSaved = SaveOffscreenShot(InShot);
		GShow.Report += cocos2d::StringUtils::format("%s %dx%d %s %d ms\n", InShot.FileName.c_str(), InShot.CaptureWidth, InShot.CaptureHeight,
			bSaved ? "offscreen" : "window", static_cast<int>(SecondsSince(Start) * 1000.0f));
		CheckTree(InShot.FileName, cocos2d::Director::getInstance()->getRunningScene());
		return bSaved;
	}

	void WriteReportAndExit()
	{
		RestoreRealFrame();
		if ( GShow.Report.empty() )
		{
			std::_Exit(0);
		}
		GShow.Report += cocos2d::StringUtils::format("total %d s, labels in buttons %d, sprites %d, failures %d\n", static_cast<int>(SecondsSince(GShow.RunStart)), GShow.CheckedLabels, GShow.CheckedSprites, GShow.Failures);
		std::ofstream(GShow.Folder + std::string(MatrixReportName)) << GShow.Report;
		std::_Exit(GShow.Failures > 0 ? MatrixFailureExitCode : 0);
	}

	void PressEscape()
	{
		cocos2d::EventKeyboard Pressed(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE, true);
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Pressed);
		cocos2d::EventKeyboard Released(cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE, false);
		cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&Released);
	}

	// Every screen at three window heights and three aspects, named <height>p_<w>x<h>_<screen>.png.
	void AddMatrixShots()
	{
		for ( const int Height : MatrixHeights )
		{
			for ( const auto& Aspect : MatrixAspects )
			{
				const int Width = Height * Aspect[0] / Aspect[1];
				const std::string Prefix = cocos2d::StringUtils::format("%dp_%dx%d_", Height, Aspect[0], Aspect[1]);
				GShow.Shots.PushBack({ [Width, Height]()
				{
					if ( cocos2d::Director::getInstance()->isPaused() )
					{
						PressEscape();
					}
					SetVirtualFrame(Width, Height);
					cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene());
				}, ScreenSettleSeconds, Prefix + "menu.png", Width, Height });
				GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::SettingsScene::CreateScene()); }, ScreenSettleSeconds, Prefix + "settings.png", Width, Height });
				GShow.Shots.PushBack({ []() { PlayLevel(BuiltinLevel(MatrixLevelIndex)); }, LevelPlaySeconds, Prefix + "level.png", Width, Height });
				GShow.Shots.PushBack({ []() { PressEscape(); }, ResizeSettleSeconds, Prefix + "pause.png", Width, Height });
			}
		}
	}

	// Builds the shots of one part. Unknown names add nothing (the run then ends at once).
	void BuildPartShots(const std::string_view InPart)
	{
		if ( InPart == "menu" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene()); }, ScreenSettleSeconds, "menu.png" });
			return;
		}
		if ( InPart == "credits" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene()); }, ScreenSettleSeconds, "menu.png" });
			GShow.Shots.PushBack({ []()
			{
				auto* const Menu = view::cocos::SceneCast<view::cocos::MenuScene>(cocos2d::Director::getInstance()->getRunningScene());
				if ( Menu )
				{
					Menu->ShowCredits();
				}
			}, ScreenSettleSeconds, "credits.png" });
			return;
		}
		if ( InPart == "menu_damaged" )
		{
			GShow.Shots.PushBack({ []()
			{
				auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
				CurrentProfile.LevelDeaths.InsertOrAssign(CurrentProfile.LevelsCompleted, 3);
				cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene());
			}, ScreenSettleSeconds, "menu_damaged.png" });
			return;
		}
		if ( InPart == "settings" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::SettingsScene::CreateScene()); }, ScreenSettleSeconds, "settings.png" });
			return;
		}
		if ( InPart == "upgrades" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::UpgradesScene::CreateScene()); }, ScreenSettleSeconds, "upgrades.png" });
			return;
		}
		if ( InPart == "editor" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::EditorScene::CreateScene()); }, ScreenSettleSeconds, "editor.png" });
			for ( const std::size_t Tab : { std::size_t{ 3 }, std::size_t{ 4 } } )
			{
				GShow.Shots.PushBack({ [Tab]()
				{
					if ( auto* const Editor = view::cocos::SceneCast<view::cocos::EditorScene>(cocos2d::Director::getInstance()->getRunningScene()) )
					{
						Editor->SelectTab(Tab);
					}
				}, ScreenSettleSeconds, Tab == 3 ? "editor_modifiers.png" : "editor_points.png" });
			}
			return;
		}
		if ( InPart == "loading" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::LoadingScene::CreateForLevel(BossLevelIndex, LoadingHoldSeconds)); }, LoadingShotSeconds, "loading.png" });
			return;
		}
		if ( InPart == "widget-editor" )
		{
			GShow.Shots.PushBack({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::WidgetEditorScene::CreateScene()); }, ScreenSettleSeconds, "widget_editor_views.png" });
			GShow.Shots.PushBack({ []()
				{
					auto* const Editor = view::cocos::WidgetEditorScene::create();
					Editor->ShowView("debug");
					cocos2d::Director::getInstance()->replaceScene(Editor);
				}, ScreenSettleSeconds, "widget_editor.png" });
			return;
		}
		if ( InPart == LookGalleryId )
		{
			GShow.Shots.PushBack({ []() { Galleries::Play(LookGalleryId); }, StayOpenSeconds, "tank_look.png" });
			return;
		}
		if ( InPart == "galleries" )
		{
			for ( const GalleryEntry& Entry : Galleries::GetAll() )
			{
				const std::string Id = Entry.Id;
				GShow.Shots.PushBack({ [Id]() { Galleries::Play(Id); }, LookGallerySeconds, "gallery_" + Id + ".png" });
			}
			return;
		}
		if ( InPart == "script" )
		{
			// The scripted scene of level 1, caught at a few moments; each shot restarts the level and waits longer.
			for ( const float Seconds : { 9.0f, 13.0f, 15.0f } )
			{
				GShow.Shots.PushBack({ []() { PlayLevel(BuiltinLevel(0)); }, Seconds, cocos2d::StringUtils::format("script_%02d.png", static_cast<int>(Seconds * 10)) });
			}
			return;
		}
		if ( InPart == "wreck" )
		{
			// Enemies destroyed on the drop level, caught while the hull flies apart, then while the wreck smokes.
			for ( const float After : { 0.15f, 0.4f, 2.5f } )
			{
				GShow.Shots.PushBack({ []() { PlayDropLevel(); }, DropKillSeconds + After, cocos2d::StringUtils::format("wreck_%03d.png", static_cast<int>(After * 100)) });
			}
			// The wreck gallery: setup after 1 s, kill after 1 s more.
			for ( const float After : { 2.3f, 4.5f } )
			{
				GShow.Shots.PushBack({ []() { Galleries::Play("wrecks"); }, After, cocos2d::StringUtils::format("wreck_gallery_%02d.png", static_cast<int>(After * 10)) });
			}
			return;
		}
		if ( InPart == "boss" )
		{
			GShow.Shots.PushBack({ []() { PlayLevel(BossLevel()); }, LevelPlaySeconds, "boss.png" });
			return;
		}
		if ( InPart == "drops" )
		{
			GShow.Shots.PushBack({ []() { PlayDropLevel(); }, DropShotSeconds, "drops.png" });
			return;
		}
		if ( InPart == "matrix" )
		{
			AddMatrixShots();
			return;
		}
		if ( InPart == "clip" )
		{
			GShow.Shots.PushBack({ []() { PlayLevel(BuiltinLevel(ClipLevelIndex)); }, ClipWarmUpSeconds, "" });
			return;
		}
		for ( int Index = 0; Index < LevelLoader::GetBuiltinCount(); ++Index )
		{
			if ( InPart == LevelPartName(Index) )
			{
				AddLevelPart(Index);
				return;
			}
		}
	}

	// Without --part: every part in one run, as the release screenshots were made.
	void BuildAllShots()
	{
		BuildPartShots("galleries");
		for ( const std::string_view Part : { "loading", "menu", "settings", "upgrades", "editor" } )
		{
			BuildPartShots(Part);
		}
		for ( int Index = 0; Index < LevelLoader::GetBuiltinCount(); ++Index )
		{
			AddLevelPart(Index);
		}
		for ( const std::string_view Part : { "boss", "drops", "clip" } )
		{
			BuildPartShots(Part);
		}
	}

	// The older single-purpose flags, each one a part.
	constexpr std::string_view LegacyFlagParts[][2] = {
		{ "--script", "script" },
		{ "--wreck", "wreck" },
		{ "--upgrades", "upgrades" },
		{ "--widget-editor", "widget-editor" },
		{ "--look-gallery", LookGalleryId },
		{ "--look", "galleries" },
	};

	void BuildShots()
	{
		const std::string Part = ParseShowcasePart(GetCommandLineA());
		if ( !Part.empty() )
		{
			BuildPartShots(Part);
			return;
		}
		for ( const auto& Legacy : LegacyFlagParts )
		{
			const std::wstring Flag(Legacy[0].begin(), Legacy[0].end());
			if ( HasArgument(Flag.c_str()) )
			{
				BuildPartShots(Legacy[1]);
				return;
			}
		}
		BuildAllShots();
	}

	std::string ClipFramePath(const int InFrame)
	{
		return GShow.Folder + cocos2d::StringUtils::format("clip/frame_%04d.png", InFrame);
	}

	void Next()
	{
		++GShow.Index;
		GShow.bShown = false;
	}

	void Tick()
	{
		if ( GShow.Index >= GShow.Shots.Size() )
		{
			if ( GShow.ClipCaptured >= GShow.ClipFrames )
			{
				WriteReportAndExit();
			}
			return;
		}

		cocos2d::Director* const Director = cocos2d::Director::getInstance();
		if ( GShow.bCapturing && Director->isPaused() )
		{
			// The saved-file callback comes through the scheduler, which a paused Director no longer updates.
			Director->getScheduler()->update(0.0f);
			return;
		}
		const Shot& CurrentShot = GShow.Shots[GShow.Index];
		if ( !GShow.bShown )
		{
			CurrentShot.Show();
			GShow.bShown = true;
			GShow.ShownAt = Clock::now();
			return;
		}
		if ( GShow.bCapturing || SecondsSince(GShow.ShownAt) < CurrentShot.WaitSeconds )
		{
			return;
		}

		if ( CurrentShot.CaptureWidth > 0 && CaptureMatrixShot(CurrentShot) )
		{
			Next();
			return;
		}
		if ( !CurrentShot.FileName.empty() )
		{
			GShow.bCapturing = true;
			cocos2d::utils::captureScreen([](bool, const std::string&) { GShow.bCapturing = false; Next(); }, GShow.Folder + CurrentShot.FileName);
			return;
		}

		// The clip: every drawn frame is captured, and the game is slowed so that each one advances it by
		// exactly 1/ClipFramesPerSecond (a capture takes longer than a frame of the video).
		auto Scheduler = cocos2d::Director::getInstance()->getScheduler();
		const int TotalFrames = static_cast<int>(ClipSeconds * ClipFramesPerSecond);
		if ( GShow.ClipFrames >= TotalFrames )
		{
			Scheduler->setTimeScale(1.0f);
			Next();
			return;
		}
		// The engine keeps one capture at a time: the next one waits for the previous callback.
		if ( GShow.ClipCaptured < GShow.ClipFrames )
		{
			return;
		}
		if ( GShow.ClipFrames > 0 )
		{
			const float FrameSeconds = std::max(MinFrameSeconds, SecondsSince(GShow.ClipStart));
			Scheduler->setTimeScale((1.0f / ClipFramesPerSecond) / FrameSeconds);
		}
		GShow.ClipStart = Clock::now();
		cocos2d::utils::captureScreen([](bool, const std::string&) { ++GShow.ClipCaptured; }, ClipFramePath(GShow.ClipFrames));
		++GShow.ClipFrames;
	}
}

bool IsShowcaseMode(std::string& InFolder)
{
	if ( !bHasDebugTools )
	{
		return false;
	}
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	// A Unicode application: the CRT fills only the wide argument list (__argv stays null).
	if ( !__wargv )
	{
		return false;
	}
	if ( HasArgument(L"--look-gallery") )
	{
		InFolder.clear();
		return true;
	}
	for ( int Index = 1; Index + 1 < __argc; ++Index )
	{
		if ( std::wstring(__wargv[Index]) == L"--showcase" )
		{
			const int ByteCount = WideCharToMultiByte(CP_UTF8, 0, __wargv[Index + 1], -1, nullptr, 0, nullptr, nullptr);
			std::string Utf8Folder(ByteCount > 0 ? ByteCount - 1 : 0, '\0');
			WideCharToMultiByte(CP_UTF8, 0, __wargv[Index + 1], -1, &Utf8Folder[0], ByteCount, nullptr, nullptr);
			InFolder = Utf8Folder;
			return true;
		}
	}
#endif
	return false;
}

std::string ParseShowcaseLanguage(std::string_view InCommandLine)
{
	constexpr std::string_view Key = "--language=";
	const std::size_t Start = InCommandLine.find(Key);
	if ( Start == std::string_view::npos )
	{
		return "uk";
	}
	const std::size_t ValueStart = Start + Key.size();
	return std::string(InCommandLine.substr(ValueStart, InCommandLine.find(' ', ValueStart) - ValueStart));
}

std::optional<PixelSize> ParseShowcaseWindow(std::string_view InCommandLine)
{
	constexpr std::string_view Key = "--window=";
	const std::size_t Start = InCommandLine.find(Key);
	if ( Start == std::string_view::npos )
	{
		return std::nullopt;
	}
	const std::string Value(InCommandLine.substr(Start + Key.size(), InCommandLine.find(' ', Start) - Start - Key.size()));
	int Width = 0;
	int Height = 0;
	char Separator = 0;
	std::istringstream Stream(Value);
	if ( !(Stream >> Width >> Separator >> Height) || (Separator != 'x' && Separator != 'X') || Width <= 0 || Height <= 0 )
	{
		return std::nullopt;
	}
	return PixelSize{ Width, Height };
}

void RunShowcase(std::string_view InFolder)
{
	GShow = ShowcaseRun();
	GShow.RunStart = Clock::now();
	GShow.Folder = InFolder;
	if ( !GShow.Folder.empty() && GShow.Folder.back() != '/' && GShow.Folder.back() != '\\' )
	{
		GShow.Folder += '/';
	}
	if ( !GShow.Folder.empty() )
	{
		cocos2d::FileUtils::getInstance()->createDirectory(GShow.Folder + "clip/");
	}

	UseTemporaryProfile();
	MoveWindowToPrimaryScreen();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( const std::optional<PixelSize> Window = ParseShowcaseWindow(GetCommandLineA()) )
	{
		ResizeBorderlessWindow(*Window);
	}
#endif
	LoadGameData(true);
	auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
	CurrentProfile.CurrentLanguage = ParseShowcaseLanguage(GetCommandLineA());
	CurrentProfile.Coins = ShowcaseCoins;
	CurrentProfile.LevelsCompleted = 2;
	CurrentProfile.Upgrades[GameContext::Get<UpgradeManager>().GetUpgrades().Front().Id] = 1;
	if ( const std::optional<int> Style = ParseShowcaseArtStyle(GetCommandLineA()) )
	{
		CurrentProfile.ArtStyle = *Style;
	}
	GameContext::Get<ProfileManager>().ApplySettings();

	BuildShots();
	cocos2d::Director::getInstance()->runWithScene(view::cocos::MenuScene::CreateScene());
	// Ticks after every drawn frame, like the menu tests.
	cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(cocos2d::Director::EVENT_AFTER_DRAW, [](cocos2d::EventCustom*) { Tick(); });
}

} // namespace tests
} // namespace game

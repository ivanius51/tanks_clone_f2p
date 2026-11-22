#include "tests/Showcase.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <optional>
#include <sstream>
#include <vector>

#include "base/ccUtils.h"

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/GameData.h"
#include "game/LevelLoader.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Tank.h"
#include "scenes/EditorScene.h"
#include "scenes/GameScene.h"
#include "scenes/LoadingScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SettingsScene.h"
#include "scenes/UpgradesScene.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/UpgradeManager.h"
#include "core/ObjectCast.h"
#include "tests/Galleries.h"
#include "tests/TestRunner.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <stdlib.h>
#endif

namespace tanks
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

	// One thing to show: build the screen, wait, then take a picture (or record the clip).
	struct Shot
	{
		std::function<void()> Show;
		float WaitSeconds;
		std::string FileName; // empty: the clip
	};

	struct ShowcaseRun
	{
		std::string Folder;
		std::vector<Shot> Shots;
		size_t Index = 0;
		bool bShown = false;
		bool bCapturing = false;
		Clock::time_point ShownAt;
		Clock::time_point ClipStart;
		int ClipFrames = 0;
		int ClipCaptured = 0;
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
		if ( CurrentLevel.Waves.size() > 1 )
		{
			CurrentLevel.Waves.erase(CurrentLevel.Waves.begin(), CurrentLevel.Waves.end() - 1);
			CurrentLevel.Waves.front().Delay = 0.0f;
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
			std::vector<Tank*> Enemies;
			GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&Enemies](Tank& InTank)
			{
				if ( InTank.GetTeam() == Team::Enemy && !InTank.IsDestroying() )
				{
					Enemies.push_back(&InTank);
				}
			});
			for ( Tank* const Enemy : Enemies )
			{
				Enemy->ApplyDamage(DropKillDamage, Team::Player);
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

	void BuildShots()
	{
		if ( HasArgument(L"--script") )
		{
			// The scripted scene of level 1, caught at a few moments; each shot restarts the level and waits longer.
			for ( const float Seconds : { 9.0f, 13.0f, 15.0f } )
			{
				GShow.Shots.push_back({ []() { PlayLevel(BuiltinLevel(0)); }, Seconds, cocos2d::StringUtils::format("script_%02d.png", static_cast<int>(Seconds * 10)) });
			}
			return;
		}
		if ( HasArgument(L"--wreck") )
		{
			// Enemies destroyed on the drop level, caught while the hull flies apart, then while the wreck smokes.
			for ( const float After : { 0.15f, 0.4f, 2.5f } )
			{
				GShow.Shots.push_back({ []() { PlayDropLevel(); }, DropKillSeconds + After, cocos2d::StringUtils::format("wreck_%03d.png", static_cast<int>(After * 100)) });
			}
			// The wreck gallery: setup after 1 s, kill after 1 s more.
			for ( const float After : { 2.3f, 4.5f } )
			{
				GShow.Shots.push_back({ []() { Galleries::Play("wrecks"); }, After, cocos2d::StringUtils::format("wreck_gallery_%02d.png", static_cast<int>(After * 10)) });
			}
			return;
		}
		if ( HasArgument(L"--upgrades") )
		{
			GShow.Shots.push_back({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::UpgradesScene::CreateScene()); }, ScreenSettleSeconds, "upgrades.png" });
			return;
		}
		if ( HasArgument(L"--look-gallery") )
		{
			GShow.Shots.push_back({ []() { Galleries::Play(LookGalleryId); }, StayOpenSeconds, "tank_look.png" });
			return;
		}
		for ( const GalleryEntry& Entry : Galleries::GetAll() )
		{
			const std::string Id = Entry.Id;
			GShow.Shots.push_back({ [Id]() { Galleries::Play(Id); }, LookGallerySeconds, "gallery_" + Id + ".png" });
		}
		if ( HasArgument(L"--look") )
		{
			return;
		}
		GShow.Shots.push_back({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::LoadingScene::CreateForLevel(BossLevelIndex, LoadingHoldSeconds)); }, LoadingShotSeconds, "loading.png" });
		GShow.Shots.push_back({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::MenuScene::CreateScene()); }, ScreenSettleSeconds, "menu.png" });
		GShow.Shots.push_back({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::SettingsScene::CreateScene()); }, ScreenSettleSeconds, "settings.png" });
		GShow.Shots.push_back({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::UpgradesScene::CreateScene()); }, ScreenSettleSeconds, "upgrades.png" });
		GShow.Shots.push_back({ []() { cocos2d::Director::getInstance()->replaceScene(view::cocos::EditorScene::CreateScene()); }, ScreenSettleSeconds, "editor.png" });
		for ( int Index = 0; Index < LevelLoader::GetBuiltinCount(); ++Index )
		{
			const std::string CurrentName = cocos2d::StringUtils::format("level_%d.png", Index + 1);
			GShow.Shots.push_back({ [Index]() { PlayLevel(BuiltinLevel(Index)); }, LevelPlaySeconds, CurrentName });
		}
		GShow.Shots.push_back({ []() { PlayLevel(BossLevel()); }, LevelPlaySeconds, "boss.png" });
		GShow.Shots.push_back({ []() { PlayDropLevel(); }, DropShotSeconds, "drops.png" });
		GShow.Shots.push_back({ []() { PlayLevel(BuiltinLevel(ClipLevelIndex)); }, ClipWarmUpSeconds, "" });
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
		if ( GShow.Index >= GShow.Shots.size() )
		{
			if ( GShow.ClipCaptured >= GShow.ClipFrames )
			{
				std::_Exit(0);
			}
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
		CheckedCast<cocos2d::GLViewImpl>(cocos2d::Director::getInstance()->getOpenGLView())->setWindowed(Window->Width, Window->Height);
	}
#endif
	LoadGameData(true);
	auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
	CurrentProfile.CurrentLanguage = Language::Ukrainian;
	CurrentProfile.Coins = ShowcaseCoins;
	CurrentProfile.LevelsCompleted = 2;
	CurrentProfile.Upgrades[GameContext::Get().GetManager<UpgradeManager>().GetUpgrades().front().Id] = 1;
	GameContext::Get().GetManager<ProfileManager>().ApplySettings();

	BuildShots();
	cocos2d::Director::getInstance()->runWithScene(view::cocos::MenuScene::CreateScene());
	// Ticks after every drawn frame, like the menu tests.
	cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(cocos2d::Director::EVENT_AFTER_DRAW, [](cocos2d::EventCustom*) { Tick(); });
}

} // namespace tests
} // namespace tanks

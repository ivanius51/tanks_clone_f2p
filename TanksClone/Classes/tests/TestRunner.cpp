#include "tests/TestRunner.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string_view>
#include <thread>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/GameData.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/LevelLoader.h"
#include "game/ObjectsFactory.h"
#include "game/ResourceManifest.h"
#include "game/objects/Tank.h"
#include "model/Random.h"
#include "scenes/GameScene.h"
#include "services/ResourceHolder.h"
#include "services/ResourceManager.h"
#include "platform/AllocationCounter.h"
#include "platform/CrashReporter.h"
#include "tests/CrashTests.h"
#include "tests/AudioTests.h"
#include "tests/MenuTests.h"
#include "tests/TestFramework.h"
#include "tests/TestSelection.h"
#include "tests/UnitTests.h"
#include "templates/TContainers.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#endif

namespace game
{
namespace tests
{

namespace
{
	// Fixed seeds make every autotest run play out the same way.
	constexpr std::uint64_t UnitTestSeed = 20261120;
	constexpr std::uint64_t LevelTestSeed = 1000;

	// The whole run is killed after this many real seconds (--max-time=<seconds> changes it), so a hang never blocks a build.
	constexpr int DefaultMaxRunSeconds = 30;
	constexpr int TimeoutExitCode = 124;

	// How long one level may take (game seconds).
	constexpr float LevelTimeout = 150.0f;
	// The level tests advance the game in fixed steps, as many as fit the budget of a frame, so a case plays out
	// the same at any frame rate (a window in the background gets ~20 FPS).
	constexpr float FixedStep = 1.0f / 60.0f;
	constexpr int MaxStepsPerFrame = 48;
	constexpr auto StepBudget = std::chrono::milliseconds(12);

	// A level the bot is expected to win.
	struct LevelCase
	{
		std::string Name;
		LevelData Level;
		bool bLoaded = true;
		bool bMustWin = false;
		bool bStartWithoutShells = false; // the bot has to fight with the machine gun
		bool bBulletClash = false;
		bool bDriveThroughWall = false;   // no bot: the test drives the tank up through the wall row, firing the cannon
		float CrowdSeconds = 0.0f;       // > 0: a crowd test: the player cannot die, tanks must never stay inside each other
		int SimSteps = 0;                // > 0: a simulation benchmark: this many fixed steps run in a tight loop, nothing is drawn
		std::uint64_t Seed = 0;          // the same in the short and the full run, whatever cases are skipped
	};

	constexpr int WallRow = 5;
	// Two tanks may touch for a moment (one drives into the other's spawn point), never for longer.
	constexpr float MaxOverlapSeconds = 1.0f;
	constexpr int SimBenchSteps = 1000;
	constexpr int SimBenchWarmupSteps = 120;
	constexpr float SimBenchStepSeconds = 1.0f / 120.0f;
	constexpr std::int64_t SimBenchMaxLiveGrowthBytes = 8 * 1024 * 1024;

	struct RunState
	{
		std::string ReportText;
		int Failed = 0;
		TestSelection Selection;
		TArray<LevelCase> Cases;
		TArray<TestResult> Results;
		size_t Index = 0;
		view::cocos::GameScene* CurrentScene = nullptr;
		bool bFinished = false;
		bool bWon = false;
		bool bCrossed = false;
		AllocationCounter::Snapshot CrowdAllocStart;
		bool bSimBenchPending = false; // the loop runs after the scheduler's update, never inside it
		std::string SimBenchLine;
		std::string SimMemoryLine;
		std::int64_t SimLiveGrowthBytes{ 0 };
		std::chrono::steady_clock::time_point CrowdStartTime;
		std::chrono::steady_clock::time_point CrowdLastTick;
		int CrowdFrames{ 0 };
		double CrowdWorstFrameMs{ 0.0 };
		bool bPrepared = false; // the case's start conditions are applied to the new player's tank
		TMap<std::pair<const GameObject*, const GameObject*>, float> Overlaps; // how long each pair has overlapped
		float WorstOverlap = 0.0f;
		float LastElapsed = 0.0f;
		int Token = 0; // the address is used as the scheduler target
		cocos2d::EventListenerCustom* Stepper = nullptr;
	};

	RunState GState;

	LevelData OpenFieldLevel()
	{
		LevelData CurrentLevel = LevelLoader::CreateEmpty();
		CurrentLevel.Rows[GridRows - 1][GridColumns / 2] = tiles::PlayerSpawn;
		CurrentLevel.Rows[0][1] = tiles::EnemySpawn;
		CurrentLevel.TotalEnemies = 1;
		CurrentLevel.MaxAlive = 1;
		CurrentLevel.EnemyIds = { "Basic" };
		return CurrentLevel;
	}

	LevelData WallsLevel()
	{
		LevelData CurrentLevel = OpenFieldLevel();
		for ( int Column = 3; Column < 13; ++Column )
		{
			CurrentLevel.Rows[5][Column] = tiles::Brick;
		}
		CurrentLevel.TotalEnemies = 2;
		CurrentLevel.MaxAlive = 2;
		CurrentLevel.Rows[0][14] = tiles::EnemySpawn;
		return CurrentLevel;
	}

	LevelData FullWallLevel()
	{
		LevelData CurrentLevel = OpenFieldLevel();
		for ( int Column = 0; Column < GridColumns; ++Column )
		{
			CurrentLevel.Rows[WallRow][Column] = tiles::Brick;
		}
		return CurrentLevel;
	}

	float WallLineY()
	{
		return (GridRows - WallRow - 0.5f) * TileSize;
	}

	// The player of the level under test: the previous level's tank may still exist for a frame.
	Tank* FindPlayerTank()
	{
		Tank* Found = nullptr;
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&Found](Tank& InObject)
		{
			if ( InObject.GetName() == PlayerName && !InObject.IsDestroying() && InObject.GetVisualNode()
				&& InObject.GetVisualNode()->getScene() == GState.CurrentScene )
			{
				Found = &InObject;
			}
		});
		return Found;
	}

	LevelData CrowdedLevel()
	{
		LevelData CurrentLevel = LevelLoader::CreateEmpty();
		CurrentLevel.Rows[GridRows - 1][GridColumns / 2] = tiles::PlayerSpawn;
		for ( int Column = 0; Column < GridColumns; Column += 3 )
		{
			CurrentLevel.Rows[0][Column] = tiles::EnemySpawn;
			CurrentLevel.Rows[4][Column] = tiles::EnemySpawn;
		}
		CurrentLevel.Rows[7][3] = tiles::Brick;
		CurrentLevel.Rows[7][12] = tiles::Brick;
		CurrentLevel.TotalEnemies = 40;
		CurrentLevel.MaxAlive = 12;
		CurrentLevel.SpawnInterval = 0.3f;
		CurrentLevel.EnemyIds.Clear();
		for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
		{
			CurrentLevel.EnemyIds.PushBack(Enemy.Id);
		}
		return CurrentLevel;
	}

	void AddCase(const LevelCase& InCase, const std::uint64_t InSlot)
	{
		GState.Cases.PushBack(InCase);
		GState.Cases.Back().Seed = LevelTestSeed + InSlot;
	}

	void AddSimBenchCase(const int InBuiltinCount)
	{
		if ( !GState.Selection.bSimBench )
		{
			return;
		}
		LevelCase Sim;
		Sim.Name = "Simulation speed: " + std::to_string(SimBenchSteps) + " fixed steps on the crowd level, no drawing";
		Sim.Level = CrowdedLevel();
		Sim.SimSteps = SimBenchSteps;
		AddCase(Sim, InBuiltinCount + 6);
	}

	void RunSimBench(view::cocos::GameScene& InScene)
	{
		cocos2d::Scheduler* const Scheduler = cocos2d::Director::getInstance()->getScheduler();
		const float PreviousTimeScale = Scheduler->getTimeScale();
		Scheduler->setTimeScale(1.0f);
		for ( int Warmup = 0; Warmup < SimBenchWarmupSteps; ++Warmup )
		{
			Scheduler->update(SimBenchStepSeconds);
		}
		AllocationCounter::ResetPeak();
		const AllocationCounter::Snapshot Start = AllocationCounter::Capture();
		const float ElapsedBefore = InScene.GetElapsed();
		const std::chrono::steady_clock::time_point StartTime = std::chrono::steady_clock::now();
		double WorstStepMs = 0.0;
		std::chrono::steady_clock::time_point LastTick = StartTime;
		for ( int StepIndex = 0; StepIndex < SimBenchSteps; ++StepIndex )
		{
			Scheduler->update(SimBenchStepSeconds);
			const std::chrono::steady_clock::time_point Now = std::chrono::steady_clock::now();
			WorstStepMs = std::max(WorstStepMs, std::chrono::duration<double, std::milli>(Now - LastTick).count());
			LastTick = Now;
		}
		const double TotalMs = std::chrono::duration<double, std::milli>(LastTick - StartTime).count();
		Scheduler->setTimeScale(PreviousTimeScale);
		const AllocationCounter::Snapshot End = AllocationCounter::Capture();
		const double GameSeconds = InScene.GetElapsed() - ElapsedBefore;
		GState.SimBenchLine = cocos2d::StringUtils::format("sim: %d steps of %.4f s (%.1f s of game time) in %.1f ms; %.0f ms per 1000 steps, %.1fx real time, worst step %.2f ms",
			SimBenchSteps, SimBenchStepSeconds, GameSeconds, TotalMs, TotalMs * 1000.0 / SimBenchSteps, GameSeconds * 1000.0 / std::max(TotalMs, 0.001), WorstStepMs);
		if ( !AllocationCounter::IsEnabled() )
		{
			return;
		}
		const std::uint64_t Allocs = End.Allocations - Start.Allocations;
		GState.SimLiveGrowthBytes = End.LiveBytes - Start.LiveBytes;
		GState.SimMemoryLine = cocos2d::StringUtils::format("sim heap: %llu allocations (%.1f per step), %.1f MB allocated, peak live growth %.2f MB, live at the end %+.2f MB",
			static_cast<unsigned long long>(Allocs), static_cast<double>(Allocs) / SimBenchSteps,
			static_cast<double>(End.AllocatedBytes - Start.AllocatedBytes) / 1048576.0, static_cast<double>(End.PeakLiveBytes) / 1048576.0,
			static_cast<double>(GState.SimLiveGrowthBytes) / 1048576.0);
	}

	void ListenForSimBench()
	{
		static bool bListening = false;
		if ( bListening )
		{
			return;
		}
		bListening = true;
		cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(cocos2d::Director::EVENT_AFTER_DRAW, [](cocos2d::EventCustom*)
		{
			if ( !GState.bSimBenchPending || !GState.CurrentScene )
			{
				return;
			}
			GState.bSimBenchPending = false;
			RunSimBench(*GState.CurrentScene);
			GState.bFinished = true;
		});
	}

	void AddBenchmarkCase(const int InBuiltinCount)
	{
		if ( !GState.Selection.bBenchmark )
		{
			return;
		}
		LevelCase Crowd;
		Crowd.Name = "A crowd of enemies for 40 s: tanks never stay inside each other";
		Crowd.Level = CrowdedLevel();
		Crowd.CrowdSeconds = 40.0f;
		AddCase(Crowd, InBuiltinCount + 5);
	}

	void BuildCases()
	{
		const TestSelection& Selection = GState.Selection;
		const int BuiltinCount = LevelLoader::GetBuiltinCount();
		for ( int Index = 0; Selection.bCampaign && Index < BuiltinCount; ++Index )
		{
			LevelCase CurrentLevelCase;
			CurrentLevelCase.Name = "Level " + std::to_string(Index + 1) + " (" + LevelLoader::BuiltinId(Index) + ")";
			CurrentLevelCase.bLoaded = LevelLoader::Load(LevelLoader::BuiltinId(Index), CurrentLevelCase.Level);
			AddCase(CurrentLevelCase, Index);
		}

		if ( !Selection.bFields )
		{
			AddBenchmarkCase(BuiltinCount);
			AddSimBenchCase(BuiltinCount);
			return;
		}

		LevelCase Open;
		Open.Name = "Open field, one enemy";
		Open.Level = OpenFieldLevel();
		Open.bMustWin = true;
		AddCase(Open, BuiltinCount + 0);

		LevelCase Walls;
		Walls.Name = "A brick wall across the field, two enemies";
		Walls.Level = WallsLevel();
		Walls.bMustWin = true;
		AddCase(Walls, BuiltinCount + 1);

		LevelCase NoShells;
		NoShells.Name = "No shells: the bot wins with the machine gun";
		NoShells.Level = OpenFieldLevel();
		NoShells.bMustWin = true;
		NoShells.bStartWithoutShells = true;
		AddCase(NoShells, BuiltinCount + 2);

		LevelCase FullWall;
		FullWall.Name = "A brick wall from edge to edge: the bot does not get stuck";
		FullWall.Level = FullWallLevel();
		FullWall.bMustWin = true;
		AddCase(FullWall, BuiltinCount + 3);

		LevelCase Drive;
		Drive.Name = "Driving up while firing: the tank shoots a gap in the brick wall and gets through it";
		Drive.Level = FullWallLevel();
		Drive.bDriveThroughWall = true;
		AddCase(Drive, BuiltinCount + 4);

		LevelCase Clash;
		Clash.Name = "Bullet clash on: the bot still wins on a walled field";
		Clash.Level = WallsLevel();
		Clash.bMustWin = true;
		Clash.bBulletClash = true;
		AddCase(Clash, BuiltinCount + 6);

		AddBenchmarkCase(BuiltinCount);
	}

	float OverlapArea(const cocos2d::Rect& InFirst, const cocos2d::Rect& InSecond)
	{
		const float CurrentWidth = std::min(InFirst.getMaxX(), InSecond.getMaxX()) - std::max(InFirst.getMinX(), InSecond.getMinX());
		const float CurrentHeight = std::min(InFirst.getMaxY(), InSecond.getMaxY()) - std::max(InFirst.getMinY(), InSecond.getMinY());
		return CurrentWidth > 0.0f && CurrentHeight > 0.0f ? CurrentWidth * CurrentHeight : 0.0f;
	}

	// Measures how long every pair of tanks has been inside each other without a break.
	void TrackFrameTime()
	{
		const std::chrono::steady_clock::time_point Now = std::chrono::steady_clock::now();
		const double FrameMs = std::chrono::duration<double, std::milli>(Now - GState.CrowdLastTick).count();
		GState.CrowdLastTick = Now;
		++GState.CrowdFrames;
		GState.CrowdWorstFrameMs = std::max(GState.CrowdWorstFrameMs, FrameMs);
	}

	void TrackOverlaps(const float InDelta)
	{
		TArray<Tank*> Tanks;
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&Tanks](Tank& InObject)
		{
			if ( !InObject.IsDestroying() && InObject.GetVisualNode() && InObject.GetVisualNode()->getScene() == GState.CurrentScene )
			{
				Tanks.PushBack(&InObject);
			}
		});

		TMap<std::pair<const GameObject*, const GameObject*>, float> Still;
		for ( size_t Index = 0; Index < Tanks.Size(); ++Index )
		{
			for ( size_t InnerIndex = Index + 1; InnerIndex < Tanks.Size(); ++InnerIndex )
			{
				if ( OverlapArea(Tanks[Index]->GetCollisionRect(), Tanks[InnerIndex]->GetCollisionRect()) > 1.0f )
				{
					const auto Key = std::make_pair(static_cast<const GameObject*>(Tanks[Index]), static_cast<const GameObject*>(Tanks[InnerIndex]));
					const float Time = GState.Overlaps[Key] + InDelta;
					Still[Key] = Time;
					GState.WorstOverlap = std::max(GState.WorstOverlap, Time);
				}
			}
		}
		GState.Overlaps = std::move(Still);
	}

	void DriveUp(Tank& InPlayer)
	{
		if ( GState.bCrossed )
		{
			GState.bFinished = true;
			return;
		}
		if ( !InPlayer.IsRunning() )
		{
			InPlayer.OnCommandReceived(GameCommand(ECommandType::MoveForward));
		}
		InPlayer.Shoot();
	}

	// Names the tests use on purpose to check the missing-resource path.
	constexpr std::string_view DeliberatelyMissing = "no_such";

	TestResult CheckNothingMissing()
	{
		TestResult Result;
		Result.Name = "Resources: nothing the run asked for was missing (no placeholders shown)";
		const model::ResourceStatusBook& Status = GameContext::Get<ResourceManager>().GetStatus();
		for ( const std::string& Missing : Status.GetMissing() )
		{
			if ( Missing.find(DeliberatelyMissing) == std::string::npos )
			{
				Result.bPassed = false;
				Result.Failures.PushBack("missing: " + Missing);
			}
		}
		Result.Notes.PushBack(std::to_string(Status.Count(model::ResourceState::Ok)) + " resources checked ok");
		return Result;
	}

	void WriteReportAndExit()
	{
		GState.Failed += Report("Resource check", { CheckNothingMissing() }, GState.ReportText);
		GState.ReportText += std::string("\nTOTAL: ") + (GState.Failed == 0 ? "ALL PASSED" : "FAILED") + "\n";
		std::printf("\nTOTAL: %s\n", GState.Failed == 0 ? "ALL PASSED" : "FAILED");
		std::fflush(stdout);

		std::ofstream("test_report.txt") << GState.ReportText;
		std::_Exit(GState.Failed == 0 ? 0 : 1);
	}

	int ReadMaxRunSeconds()
	{
		constexpr std::string_view Key = "--max-time=";
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		const std::string_view Line(GetCommandLineA());
		const size_t At = Line.find(Key);
		if ( At != std::string_view::npos )
		{
			const int Seconds = std::atoi(Line.data() + At + Key.size());
			if ( Seconds > 0 )
			{
				return Seconds;
			}
		}
#endif
		return DefaultMaxRunSeconds;
	}

	// A separate thread: the main thread may be the one that hangs.
	void StartWatchdog()
	{
		const int Seconds = ReadMaxRunSeconds();
		std::thread([Seconds]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(Seconds));
			const std::string Message = "TIMEOUT: the test run took longer than " + std::to_string(Seconds) + " seconds and was stopped\n";
			std::fputs(Message.c_str(), stdout);
			std::fflush(stdout);
			std::ofstream("test_report.txt") << Message;
			std::_Exit(TimeoutExitCode);
		}).detach();
	}

	void StopFixedSteps()
	{
		auto Director = cocos2d::Director::getInstance();
		if ( GState.Stepper )
		{
			Director->getEventDispatcher()->removeEventListener(GState.Stepper);
			GState.Stepper = nullptr;
		}
		Director->getScheduler()->setTimeScale(1.0f);
	}

	// Runs outside the scheduler's own update (after the frame is drawn), so the scheduler is not re-entered.
	void AdvanceFixedSteps()
	{
		auto Scheduler = cocos2d::Director::getInstance()->getScheduler();
		const auto Start = std::chrono::steady_clock::now();
		Scheduler->setTimeScale(1.0f);
		for ( int Index = 0; Index < MaxStepsPerFrame && GState.Stepper && !GState.bSimBenchPending; ++Index )
		{
			Scheduler->update(FixedStep);
			if ( std::chrono::steady_clock::now() - Start > StepBudget )
			{
				break;
			}
		}
		// The engine's own update of the next frame then moves nothing: only the fixed steps advance the game.
		Scheduler->setTimeScale(GState.Stepper ? 0.0f : 1.0f);
	}

	void RunAudioThenExit()
	{
		if ( !GState.Selection.bAudio )
		{
			WriteReportAndExit();
			return;
		}
		RunAudioTests([](const TArray<TestResult>& InResults)
		{
			GState.Failed += Report("Audio tests (devices and formats)", InResults, GState.ReportText);
			WriteReportAndExit();
		});
	}

	void FinishAll()
	{
		cocos2d::Director::getInstance()->getScheduler()->unschedule("LEVEL_TESTS", &GState.Token);
		StopFixedSteps();
		if ( GState.Selection.HasLevelCases() )
		{
			GState.Failed += Report("Level tests (bot plays)", GState.Results, GState.ReportText);
		}
		if ( !GState.Selection.bMenu && !GState.Selection.bEditor )
		{
			RunAudioThenExit();
			return;
		}

		const bool bEditorOnly = !GState.Selection.bMenu;
		RunMenuTests([bEditorOnly](const TArray<TestResult>& InResults)
		{
			GState.Failed += Report(bEditorOnly ? "Editor tests" : "Menu tests (screens and buttons)", InResults, GState.ReportText);
			RunAudioThenExit();
		}, bEditorOnly);
	}

	// The tests save a profile and a custom level: keep them away from the player's real ones.
	void UseTemporaryWritableFolder()
	{
		auto Files = cocos2d::FileUtils::getInstance();
		std::string CurrentFolder = Files->getWritablePath() + "test_run/";
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		char TempPath[MAX_PATH] = "";
		// GetTempPathA returns the CurrentFolder with a trailing backslash, or 0 on failure.
		if ( GetTempPathA(MAX_PATH, TempPath) > 0 )
		{
			CurrentFolder = std::string(TempPath) + "TanksCloneTests\\";
		}
#endif
		Files->removeDirectory(CurrentFolder);
		Files->createDirectory(CurrentFolder);
		Files->setWritablePath(CurrentFolder);
	}

	void RecordCase(const LevelCase& InCase, view::cocos::GameScene* const InScene, const bool bInTimedOut)
	{
		TestResult Result;
		Result.Name = InCase.Name;

		if ( !InCase.bLoaded )
		{
			Result.Failures.PushBack("the level does not load");
		}
		else if ( !InScene )
		{
			Result.Failures.PushBack("the scene could not be created");
		}
		else
		{
			if ( InCase.bDriveThroughWall )
			{
				Result.Notes.PushBack(GState.bCrossed ? "got through the wall" : "did not get through the wall");
				if ( !GState.bCrossed )
				{
					Result.Failures.PushBack("the tank never got past the wall");
				}
				Result.bPassed = Result.Failures.IsEmpty();
				GState.Results.PushBack(Result);
				return;
			}

			if ( InCase.SimSteps > 0 )
			{
				Result.Notes.PushBack(GState.SimBenchLine);
				if ( !GState.SimMemoryLine.empty() )
				{
					Result.Notes.PushBack(GState.SimMemoryLine);
				}
				if ( GState.SimLiveGrowthBytes > SimBenchMaxLiveGrowthBytes )
				{
					Result.Failures.PushBack("live heap kept growing during the simulation");
				}
				Result.bPassed = Result.Failures.IsEmpty();
				GState.Results.PushBack(Result);
				return;
			}

			if ( InCase.CrowdSeconds > 0.0f )
			{
				const std::string Line = cocos2d::StringUtils::format("%.0f s, %d enemies appeared, %d destroyed; longest overlap of two tanks: %.2f s",
					InScene->GetElapsed(), InScene->GetSpawner() ? InScene->GetSpawner()->GetSpawned() : 0,
					InScene->GetSession().Kills, GState.WorstOverlap);
				Result.Notes.PushBack(Line);
				if ( GState.CrowdFrames > 0 )
				{
					const double TotalMs = std::chrono::duration<double, std::milli>(GState.CrowdLastTick - GState.CrowdStartTime).count();
					const double AverageMs = TotalMs / GState.CrowdFrames;
					Result.Notes.PushBack(cocos2d::StringUtils::format("speed: %d frames in %.0f ms; average %.3f ms per frame (%.0f ms per 1000 frames, %.0f fps), worst frame %.2f ms",
						GState.CrowdFrames, TotalMs, AverageMs, AverageMs * 1000.0, 1000.0 / std::max(AverageMs, 0.0001), GState.CrowdWorstFrameMs));
				}
				if ( AllocationCounter::IsEnabled() )
				{
					const AllocationCounter::Snapshot End = AllocationCounter::Capture();
					const float Seconds = std::max(InScene->GetElapsed(), 0.001f);
					const std::uint64_t Allocs = End.Allocations - GState.CrowdAllocStart.Allocations;
					const std::uint64_t Bytes = End.AllocatedBytes - GState.CrowdAllocStart.AllocatedBytes;
					const std::string MemoryLine = cocos2d::StringUtils::format("heap: %llu allocations (%.0f per s), %.1f MB allocated, peak live %.1f MB; peak stack %.0f KB",
						static_cast<unsigned long long>(Allocs), static_cast<double>(Allocs) / Seconds,
						static_cast<double>(Bytes) / 1048576.0, static_cast<double>(End.PeakLiveBytes) / 1048576.0,
						static_cast<double>(AllocationCounter::GetPeakStackBytes()) / 1024.0);
					Result.Notes.PushBack(MemoryLine);
				}
				if ( GState.WorstOverlap > MaxOverlapSeconds )
				{
					Result.Failures.PushBack("two tanks stayed inside each other");
				}
				Result.bPassed = Result.Failures.IsEmpty();
				GState.Results.PushBack(Result);
				return;
			}

			const model::GameSession& CurrentSession = InScene->GetSession();
			const int CurrentSpawned = InScene->GetSpawner() ? InScene->GetSpawner()->GetSpawned() : 0;
			const char* Outcome = bInTimedOut ? "TIMEOUT" : (InScene->WasWon() ? "WON" : "LOST");

			const std::string Line = cocos2d::StringUtils::format("%s in %.0f s: killed %d/%d, %d enemies appeared, wave %d/%d, drops %d (picked %d)",
				Outcome, InScene->GetElapsed(), CurrentSession.Kills, CurrentSession.TotalEnemies, CurrentSpawned, CurrentSession.WaveNumber, CurrentSession.WaveCount, CurrentSession.Drops, CurrentSession.DropsPicked);
			Result.Notes.PushBack(Line);

			if ( CurrentSpawned == 0 )
			{
				Result.Failures.PushBack("no enemy ever appeared: the spawners do not work");
			}
			if ( !bInTimedOut && InScene->WasWon() && CurrentSession.Defeated != CurrentSession.TotalEnemies )
			{
				Result.Failures.PushBack("won, but not all enemies were destroyed");
			}
			if ( InCase.bMustWin && !(InScene->IsFinished() && InScene->WasWon()) )
			{
				Result.Failures.PushBack(std::string("the bot was expected to win, but the result is ") + Outcome);
			}
			if ( bInTimedOut )
			{
				Result.Notes.PushBack("not finished in time (the bot may simply be too weak for this level)");
			}
			if ( InCase.bStartWithoutShells && !(InScene->GetBot() && InScene->GetBot()->HasUsedFreeWeapon()) )
			{
				Result.Failures.PushBack("the bot never switched to the machine gun");
			}
		}

		Result.bPassed = Result.Failures.IsEmpty();
		GState.Results.PushBack(Result);
	}

	void Step()
	{
		if ( !GState.CurrentScene )
		{
			if ( GState.Index >= GState.Cases.Size() )
			{
				FinishAll();
				return;
			}

			const LevelCase& CurrentLevelCase = GState.Cases[GState.Index];
			if ( !CurrentLevelCase.bLoaded )
			{
				RecordCase(CurrentLevelCase, nullptr, false);
				++GState.Index;
				return;
			}

			model::GameRandom().SetSeed(GState.Cases[GState.Index].Seed);
			auto RunScene = CheckedCast<view::cocos::GameScene>(view::cocos::GameScene::CreateForTest(CurrentLevelCase.Level, !CurrentLevelCase.bDriveThroughWall));
			if ( !RunScene )
			{
				RecordCase(CurrentLevelCase, nullptr, false);
				++GState.Index;
				return;
			}

			GState.CurrentScene = RunScene;
			GState.bFinished = false;
			GState.bCrossed = false;
			GState.bPrepared = false;
			GState.Overlaps.Clear();
			GState.WorstOverlap = 0.0f;
			GState.LastElapsed = 0.0f;
			RunScene->OnFinished = [](const bool bInWon) { GState.bFinished = true; GState.bWon = bInWon; };
			cocos2d::Director::getInstance()->replaceScene(RunScene);
			return;
		}

		if ( Tank* Player = FindPlayerTank() )
		{
			if ( !GState.bPrepared )
			{
				GState.bPrepared = true;
				Player->SetBulletClash(GState.Cases[GState.Index].bBulletClash);
				if ( GState.Cases[GState.Index].bStartWithoutShells )
				{
					Player->SetAmmo(0);
				}
				if ( GState.Cases[GState.Index].CrowdSeconds > 0.0f )
				{
					// Its own id: the bot's shield skill restarts a buff with the same id as a timed one.
					BuffDef Shield;
					Shield.Id = FPersistentString{ "test_invulnerable" };
					Shield.bInvulnerable = true;
					Shield.Duration = -1.0f;
					Player->GetBuffs().Add(Shield);
					Player->RefreshStats();
					AllocationCounter::ResetPeak();
					GState.CrowdAllocStart = AllocationCounter::Capture();
					GState.CrowdStartTime = std::chrono::steady_clock::now();
					GState.CrowdLastTick = GState.CrowdStartTime;
					GState.CrowdFrames = 0;
					GState.CrowdWorstFrameMs = 0.0;
				}
			}
			if ( Player->GetVisualNode() && Player->GetVisualNode()->getPositionY() > WallLineY() + TileSize / 2 )
			{
				GState.bCrossed = true;
			}
			if ( GState.Cases[GState.Index].bDriveThroughWall )
			{
				DriveUp(*Player);
			}
		}

		if ( GState.Cases[GState.Index].SimSteps > 0 && GState.bPrepared && !GState.bFinished && !GState.bSimBenchPending )
		{
			GState.bSimBenchPending = true;
			ListenForSimBench();
		}

		const float CurrentCrowdSeconds = GState.Cases[GState.Index].CrowdSeconds;
		if ( CurrentCrowdSeconds > 0.0f )
		{
			TrackOverlaps(GState.CurrentScene->GetElapsed() - GState.LastElapsed);
			TrackFrameTime();
			if ( GState.CurrentScene->GetElapsed() >= CurrentCrowdSeconds )
			{
				GState.bFinished = true;
			}
		}
		GState.LastElapsed = GState.CurrentScene->GetElapsed();

		const bool bTimedOut = GState.CurrentScene->GetElapsed() > LevelTimeout;
		if ( GState.bFinished || bTimedOut )
		{
			RecordCase(GState.Cases[GState.Index], GState.CurrentScene, bTimedOut && !GState.bFinished);
			GState.CurrentScene = nullptr;
			++GState.Index;
		}
	}

	void StartLevelTests()
	{
		BuildCases();
		auto Director = cocos2d::Director::getInstance();
		Director->getScheduler()->setTimeScale(0.0f);
		GState.Stepper = Director->getEventDispatcher()->addCustomEventListener(cocos2d::Director::EVENT_AFTER_DRAW,
			[](cocos2d::EventCustom*) { AdvanceFixedSteps(); });
		Director->getScheduler()->schedule([](float) { Step(); }, &GState.Token,
			0.0f, CC_REPEAT_FOREVER, 0.0f, false, "LEVEL_TESTS");
	}

	// The first scene: runs the unit tests (objects need a running scene), then hands over to the level tests.
	class UnitTestScene : public cocos2d::Scene
	{
	public:
		CREATE_FUNC(UnitTestScene);

		void onEnter() override
		{
			cocos2d::Scene::onEnter();

			LoadGameData(true);

			auto CurrentSandbox = cocos2d::Node::create();
			addChild(CurrentSandbox);

			if ( GState.Selection.bUnit )
			{
				const ResourceHolder EnemyAtlases(ResourceManifest().GetEnemyAtlases());
				model::GameRandom().SetSeed(UnitTestSeed);
				const TestSuite Suite = CreateUnitTests();
				GState.Failed += Report("Unit tests", Suite.Run(CurrentSandbox), GState.ReportText);
			}
			if ( GState.Selection.bCrash )
			{
				GState.Failed += Report("Crash tests (expected crashes in child processes)", RunCrashTests(), GState.ReportText);
			}

			runAction(cocos2d::Sequence::create(
				cocos2d::DelayTime::create(0.1f),
				cocos2d::CallFunc::create([]() { StartLevelTests(); }),
				nullptr));
		}
	};
}

bool HasCommandLineArgument(std::string_view InArgument)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	return std::string_view(GetCommandLineA()).find(InArgument) != std::string_view::npos;
#else
	return false;
#endif
}

bool IsTestMode()
{
	if ( !bHasDebugTools )
	{
		return false;
	}
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( std::strstr(GetCommandLineA(), "--test") )
	{
		return true;
	}
#endif
	return std::getenv("TANKS_TEST") != nullptr;
}

bool IsBackgroundMode()
{
	return bHasDebugTools && HasCommandLineArgument("--background");
}

void UseTemporaryProfile()
{
	UseTemporaryWritableFolder();
}

void RunAutomaticTests()
{
	UseTemporaryWritableFolder();
	StartWatchdog();
	CrashReporter::SetPrefixText(&GState.ReportText);
	CrashReporter::SetExtraReportFile("test_report.txt");
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	GState.Selection = TestSelection::Parse(GetCommandLineA());
#endif
	if ( GState.Selection.IsEmpty() )
	{
		GState.Selection = TestSelection::Parse("--test");
	}
	if ( !GState.Selection.bValid )
	{
		++GState.Failed;
		GState.ReportText += "Unknown --test mode: use unit, menu, editor, audio, benchmark, simbench, campaign, crash or full; running the short set.\n";
	}
	cocos2d::Director::getInstance()->runWithScene(UnitTestScene::create());
}

} // namespace tests
} // namespace game

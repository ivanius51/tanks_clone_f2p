#include "tests/AudioTests.h"

#include <chrono>
#include <memory>
#include <string>

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/AudioManager.h"
#include "services/MusicManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"

namespace tanks
{
namespace tests
{

namespace
{
	using cocos2d::experimental::AudioEngine;
	typedef std::chrono::steady_clock Clock;

	constexpr float TimeoutSeconds = 5.0f;
	constexpr float UncacheWaitSeconds = 0.5f;
	constexpr float TestVolume = 0.3f;
	const std::vector<std::string> GFormats = {
		"sounds/test/pcm8_mono_11k.wav", "sounds/test/pcm16_mono_48k.wav", "sounds/test/pcm16_stereo_44k.wav",
		"sounds/test/vorbis_stereo.ogg", "sounds/test/mp3_mono.mp3"
	};
	const std::string GCorrupt = "sounds/test/corrupt.wav";
	const std::string GMissing = "sounds/test/no_such_file.wav";

	// What the running step waits for; the engine callbacks fill it in.
	struct Waiting
	{
		bool bFinished = false;
		bool bLoaded = false;
		bool bLoadAnswered = false;
		int AudioId = AudioEngine::INVALID_AUDIO_ID;
	};

	struct Step
	{
		std::string Name;
		std::function<void(TestContext&)> Run;
		std::function<bool(TestContext&)> IsDone;
	};

	struct AudioRun
	{
		std::vector<Step> Steps;
		std::vector<TestResult> Results;
		std::function<void(const std::vector<TestResult>&)> OnDone;
		cocos2d::EventListenerCustom* Listener = nullptr;
		size_t Index = 0;
		bool bActing = true;
		bool bSoundWasEnabled = true;
		Clock::time_point StepStart;
		std::unique_ptr<TestContext> Context;
		std::shared_ptr<Waiting> State;
	};

	AudioRun GRun;

	float SecondsSince(const Clock::time_point InStart)
	{
		return std::chrono::duration<float>(Clock::now() - InStart).count();
	}

	bool IsFinished(TestContext&)
	{
		return GRun.State->bFinished;
	}

	void PlayAndWatch(TestContext& InContext, const std::string& InPath)
	{
		const std::shared_ptr<Waiting> State = GRun.State;
		State->AudioId = AudioEngine::play2d(InPath, false, TestVolume);
		InContext.Expect(State->AudioId != AudioEngine::INVALID_AUDIO_ID, "play2d returns an id");
		if ( State->AudioId == AudioEngine::INVALID_AUDIO_ID )
		{
			State->bFinished = true;
			return;
		}
		AudioEngine::setFinishCallback(State->AudioId, [State](int, const std::string&) { State->bFinished = true; });
	}

	void PreloadAndWatch(const std::string& InPath)
	{
		const std::shared_ptr<Waiting> State = GRun.State;
		AudioEngine::preload(InPath, [State](const bool bInOk)
		{
			State->bLoadAnswered = true;
			State->bLoaded = bInOk;
		});
	}

	Step PlayAtOnce(const std::string& InPath)
	{
		return { "play without preload: " + InPath, [InPath](TestContext& InContext)
			{
				AudioEngine::uncache(InPath);
				PlayAndWatch(InContext, InPath);
			}, IsFinished };
	}

	Step PreloadThenPlay(const std::string& InPath)
	{
		return { "preload in background, then play: " + InPath, [InPath](TestContext&)
			{
				AudioEngine::uncache(InPath);
				PreloadAndWatch(InPath);
			}, [InPath](TestContext& InContext)
			{
				const Waiting& State = *GRun.State;
				if ( State.bLoadAnswered && State.AudioId == AudioEngine::INVALID_AUDIO_ID && !State.bFinished )
				{
					InContext.Expect(State.bLoaded, "the preload succeeds");
					PlayAndWatch(InContext, InPath);
				}
				return GRun.State->bFinished;
			} };
	}

	Step DeviceStep(const std::string& InDevice)
	{
		return { "device: " + (InDevice.empty() ? std::string("system default") : InDevice), [InDevice](TestContext& InContext)
			{
				auto& Audio = GameContext::Get().GetManager<AudioManager>();
				Audio.SetDevice(InDevice);
				const std::string Opened = Audio.GetOpenedDevice();
				InContext.Note("opened: " + Opened);
				InContext.Expect(!Opened.empty(), "a device is open");
				if ( !InDevice.empty() )
				{
					InContext.ExpectEqual(Opened, InDevice, "the chosen device is open");
				}
				PlayAndWatch(InContext, GFormats[2]);
			}, IsFinished };
	}

	void AddDeviceSteps()
	{
		const std::vector<std::string> Devices = GameContext::Get().GetManager<AudioManager>().GetDevices();
		GRun.Steps.push_back({ "output devices are listed", [Devices](TestContext& InContext)
			{
				InContext.Expect(!Devices.empty(), "at least one output device");
				for ( const std::string& Device : Devices )
				{
					InContext.Note(Device);
				}
			}, [](TestContext&) { return true; } });
		GRun.Steps.push_back(DeviceStep(""));
		for ( const std::string& Device : Devices )
		{
			GRun.Steps.push_back(DeviceStep(Device));
		}
		GRun.Steps.push_back({ "back to the system default", [](TestContext&) { GameContext::Get().GetManager<AudioManager>().SetDevice(""); },
			[](TestContext&) { return true; } });
	}

	void AddFileSteps()
	{
		for ( const std::string& Path : GFormats )
		{
			GRun.Steps.push_back(PlayAtOnce(Path));
			GRun.Steps.push_back(PreloadThenPlay(Path));
		}
		GRun.Steps.push_back({ "a broken file fails to load", [](TestContext&) { PreloadAndWatch(GCorrupt); }, [](TestContext& InContext)
			{
				if ( GRun.State->bLoadAnswered )
				{
					InContext.Expect(!GRun.State->bLoaded, "the preload reports failure");
				}
				return GRun.State->bLoadAnswered;
			} });
		GRun.Steps.push_back({ "a missing file is not played", [](TestContext& InContext)
			{
				InContext.Expect(AudioEngine::play2d(GMissing) == AudioEngine::INVALID_AUDIO_ID, "play2d gives no id");
			}, [](TestContext&) { return true; } });
		GRun.Steps.push_back({ "freeing a sound while it plays", [](TestContext& InContext)
			{
				PlayAndWatch(InContext, GFormats[2]);
				AudioEngine::uncache(GFormats[2]);
			}, [](TestContext&) { return SecondsSince(GRun.StepStart) > UncacheWaitSeconds; } });
		GRun.Steps.push_back({ "game sound through AudioManager", [](TestContext& InContext)
			{
				auto& Audio = GameContext::Get().GetManager<AudioManager>();
				Audio.SetEnabled(true);
				Audio.Play(ids::sounds::UiClick);
				const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::sounds::UiClick, model::ResourceKind::Sound);
				InContext.Expect(!Path.empty(), "the click resolves to a file");
				PlayAndWatch(InContext, Path);
			}, IsFinished });
		GRun.Steps.push_back({ "music plays and turns intense", [](TestContext& InContext)
			{
				auto& Music = GameContext::Get().GetManager<MusicManager>();
				Music.Request(model::MusicMood::Calm, true);
				InContext.Expect(!Music.GetCurrentTrack().empty(), "a calm track is chosen");
				Music.Request(model::MusicMood::Intense);
				InContext.Expect(Music.GetMood() == model::MusicMood::Intense, "the mood turns intense");
			}, [](TestContext&) { return SecondsSince(GRun.StepStart) > UncacheWaitSeconds; } });
		GRun.Steps.push_back({ "music stops", [](TestContext& InContext)
			{
				auto& Music = GameContext::Get().GetManager<MusicManager>();
				Music.Request(model::MusicMood::Silent);
				InContext.Expect(Music.GetCurrentTrack().empty(), "no track after silence");
			}, [](TestContext&) { return true; } });
	}

	void Finish()
	{
		cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(GRun.Listener);
		GRun.Listener = nullptr;
		AudioEngine::stopAll();
		auto& Audio = GameContext::Get().GetManager<AudioManager>();
		Audio.SetDevice(GameContext::Get().GetManager<ProfileManager>().Get().AudioDevice);
		Audio.SetEnabled(GRun.bSoundWasEnabled);
		auto CurrentOnDone = GRun.OnDone;
		CurrentOnDone(GRun.Results);
	}

	void Record(const bool bInTimedOut)
	{
		TestResult Result;
		Result.Name = GRun.Steps[GRun.Index].Name;
		Result.Failures = GRun.Context->GetFailures();
		Result.Notes = GRun.Context->GetNotes();
		if ( bInTimedOut )
		{
			Result.Failures.push_back("the sound did not finish in time");
		}
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
			GRun.State = std::make_shared<Waiting>();
			GRun.StepStart = Clock::now();
			CurrentStep.Run(*GRun.Context);
			GRun.bActing = false;
			return;
		}
		const bool bDone = CurrentStep.IsDone(*GRun.Context);
		if ( bDone || SecondsSince(GRun.StepStart) > TimeoutSeconds )
		{
			Record(!bDone);
			++GRun.Index;
			GRun.bActing = true;
		}
	}
}

void RunAudioTests(const std::function<void(const std::vector<TestResult>&)>& InOnDone)
{
	GRun = AudioRun();
	GRun.OnDone = InOnDone;
	GRun.bSoundWasEnabled = GameContext::Get().GetManager<AudioManager>().IsEnabled();
	AddDeviceSteps();
	AddFileSteps();
	// Ticks after every drawn frame, like the menu tests.
	GRun.Listener = cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(
		cocos2d::Director::EVENT_AFTER_DRAW, [](cocos2d::EventCustom*) { Tick(); });
}

} // namespace tests
} // namespace tanks

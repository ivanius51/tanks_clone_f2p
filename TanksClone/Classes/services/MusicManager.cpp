#include "services/ConfigRefs.h"
#include "services/MusicManager.h"

#include <algorithm>
#include <optional>
#include <string_view>

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/AudioManager.h"
#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	const char* const UpdateKey = "MUSIC_UPDATE";

	TArray<std::string> ReadList(const model::IConfigNode& InNode, const std::string_view InKey)
	{
		TArray<std::string> Result;
		if ( const model::IConfigNode* const List = InNode.FindChild(InKey) )
		{
			for ( size_t Index = 0; Index < List->GetItemCount(); ++Index )
			{
				if ( const std::optional<std::string> Id = List->GetItem(Index)->ReadString() )
				{
					Result.PushBack(*Id);
				}
			}
		}
		return Result;
	}
}

MusicManager::~MusicManager()
{
	cocos2d::Director::getInstance()->getScheduler()->unschedule(UpdateKey, this);
	StopAll();
}

void MusicManager::Init()
{
	model::MusicSettings Settings;
	if ( const model::IConfigNode* const Node = config_refs::FindChild(ids::configs::Music, "music") )
	{
		Settings.MenuTracks = ReadList(*Node, "menu");
		Settings.CalmTracks = ReadList(*Node, "calm");
		Settings.IntenseTracks = ReadList(*Node, "intense");
		MenuTracks = Settings.MenuTracks;
	}
	const model::IConfigNode& Rules = config_refs::GetConfig(ids::configs::MusicRules);
	Settings.FadeSeconds = Rules.GetFloat("fade_seconds");
	Settings.MinIntenseSeconds = Rules.GetFloat("min_intense_seconds");
	Settings.IntenseProgress = Rules.GetFloat("intense_progress");
	PausedVolume = Rules.GetFloat("paused_volume");
	MenuVolume = Rules.GetFloat("menu_volume");
	Director = MakeUnique<model::MusicDirector>(Settings);
	cocos2d::Director::getInstance()->getScheduler()->schedule([this](const float InDelta) { Update(InDelta); }, this, 0.0f, false, UpdateKey);
}

void MusicManager::Request(const model::MusicMood InMood, const bool bInNewTrack)
{
	if ( Director )
	{
		Director->Request(InMood, bInNewTrack);
	}
}

void MusicManager::PlayTrack(const std::string& InTrack)
{
	if ( Director )
	{
		Director->PlayTrack(InTrack);
	}
}

model::MusicMood MusicManager::GetMood() const
{
	return Director ? Director->GetMood() : model::MusicMood::Silent;
}

float MusicManager::GetIntenseProgress() const
{
	return Director ? Director->GetSettings().IntenseProgress : 1.0f;
}

void MusicManager::SetVolume(const float InVolume)
{
	Volume = std::clamp(InVolume, 0.0f, 1.0f);
}

void MusicManager::SetPaused(const bool bInPaused)
{
	bPaused = bInPaused;
	// The engine stops the scheduler while the game is paused, so the new loudness is applied right away.
	Update(0.0f);
}

float MusicManager::GetVolume() const
{
	return Volume;
}

void MusicManager::Update(const float InDelta)
{
	if ( !Director )
	{
		return;
	}
	Director->Update(InDelta);
	const float Master = GameContext::Get<AudioManager>().IsEnabled() ? Volume * (bPaused ? PausedVolume : 1.0f) : 0.0f;
	if ( Master <= 0.0f )
	{
		StopAll();
		return;
	}
	const model::MusicVoice& Current = Director->GetCurrent();
	const model::MusicVoice& Previous = Director->GetPrevious();
	Tracks.Erase(std::remove_if(Tracks.begin(), Tracks.end(), [&](const Playing& InPlaying)
	{
		const bool bWanted = InPlaying.Track == Current.Track || InPlaying.Track == Previous.Track;
		if ( !bWanted )
		{
			GameContext::Get().GetAudioOutput().StopLoop(InPlaying.Id);
		}
		return !bWanted;
	}), Tracks.end());
	Sync(Current, Master);
	Sync(Previous, Master);
}

const std::string& MusicManager::GetCurrentTrack() const
{
	static const std::string None;
	return Director ? Director->GetCurrent().Track : None;
}

void MusicManager::Sync(const model::MusicVoice& InVoice, const float InMaster)
{
	if ( InVoice.Track.empty() )
	{
		return;
	}
	ports::IAudioOutput& Output = GameContext::Get().GetAudioOutput();
	const bool bMenuTrack = std::find(MenuTracks.begin(), MenuTracks.end(), InVoice.Track) != MenuTracks.end();
	const float Loudness = InVoice.Volume * InMaster * (bMenuTrack ? MenuVolume : 1.0f);
	const auto Found = std::find_if(Tracks.begin(), Tracks.end(), [&](const Playing& InPlaying) { return InPlaying.Track == InVoice.Track; });
	if ( Found != Tracks.end() )
	{
		Output.SetLoopVolume(Found->Id, Loudness);
		return;
	}
	const std::string Path = TrackPath(InVoice.Track);
	const int Id = Path.empty() ? -1 : Output.PlayLoop(Path, Loudness);
	if ( Id >= 0 )
	{
		Tracks.PushBack({ InVoice.Track, Id });
	}
}

void MusicManager::StopAll()
{
	for ( const Playing& Track : Tracks )
	{
		GameContext::Get().GetAudioOutput().StopLoop(Track.Id);
	}
	Tracks.Clear();
}

std::string MusicManager::TrackPath(const std::string& InTrackId) const
{
	return std::string(GameContext::Get<ResourceManager>().ResolvePath(model::ResourceId(InTrackId), model::ResourceKind::Sound).View());
}

} // namespace game

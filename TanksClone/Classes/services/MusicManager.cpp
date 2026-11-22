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

namespace tanks
{

namespace
{
	const char* const UpdateKey = "MUSIC_UPDATE";

	std::vector<std::string> ReadList(const model::IConfigNode& InNode, const std::string_view InKey)
	{
		std::vector<std::string> Result;
		if ( const model::IConfigNode* const List = InNode.FindChild(InKey) )
		{
			for ( size_t Index = 0; Index < List->GetItemCount(); ++Index )
			{
				if ( const std::optional<std::string> Id = List->GetItem(Index)->ReadString() )
				{
					Result.push_back(*Id);
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
	const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::Music, model::ResourceKind::Config);
	if ( const model::IConfigNode* const Node = GameContext::Get().GetManager<ConfigReader>().GetConfig(Path).FindChild("music") )
	{
		Settings.MenuTracks = ReadList(*Node, "menu");
		Settings.CalmTracks = ReadList(*Node, "calm");
		Settings.IntenseTracks = ReadList(*Node, "intense");
		Settings.FadeSeconds = Node->GetFloat("fade_seconds");
		Settings.MinIntenseSeconds = Node->GetFloat("min_intense_seconds");
		Settings.IntenseProgress = Node->GetFloat("intense_progress");
		PausedVolume = Node->GetFloat("paused_volume");
		MenuVolume = Node->GetFloat("menu_volume");
		MenuTracks = Settings.MenuTracks;
	}
	Director = std::make_unique<model::MusicDirector>(Settings);
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
	const float Master = GameContext::Get().GetManager<AudioManager>().IsEnabled() ? Volume * (bPaused ? PausedVolume : 1.0f) : 0.0f;
	if ( Master <= 0.0f )
	{
		StopAll();
		return;
	}
	const model::MusicVoice& Current = Director->GetCurrent();
	const model::MusicVoice& Previous = Director->GetPrevious();
	Tracks.erase(std::remove_if(Tracks.begin(), Tracks.end(), [&](const Playing& InPlaying)
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
		Tracks.push_back({ InVoice.Track, Id });
	}
}

void MusicManager::StopAll()
{
	for ( const Playing& Track : Tracks )
	{
		GameContext::Get().GetAudioOutput().StopLoop(Track.Id);
	}
	Tracks.clear();
}

std::string MusicManager::TrackPath(const std::string& InTrackId) const
{
	return GameContext::Get().GetManager<ResourceManager>().ResolvePath(model::ResourceId(InTrackId), model::ResourceKind::Sound);
}

} // namespace tanks

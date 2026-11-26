#include "model/MusicDirector.h"
#include "templates/TContainers.h"

#include <algorithm>

namespace game::model
{

namespace
{
	float StepTowards(const float InValue, const float InTarget, const float InStep)
	{
		return InValue < InTarget ? std::min(InTarget, InValue + InStep) : std::max(InTarget, InValue - InStep);
	}
}

MusicDirector::MusicDirector(const MusicSettings& InSettings) : Settings(InSettings)
{
}

void MusicDirector::PlayTrack(const std::string& InTrack)
{
	Mood = MusicMood::Calm;
	Pending = MusicMood::Calm;
	bPinned = true;
	Switch(InTrack);
}

void MusicDirector::Request(const MusicMood InMood, const bool bInNewTrack)
{
	const bool bInGame = InMood == MusicMood::Calm || InMood == MusicMood::Intense;
	if ( bPinned && bInGame && !bInNewTrack )
	{
		return;
	}
	bPinned = false;
	Pending = InMood;
	if ( InMood == Mood && !bInNewTrack )
	{
		return;
	}
	if ( Mood == MusicMood::Intense && InMood == MusicMood::Calm && IntenseTime < Settings.MinIntenseSeconds && !bInNewTrack )
	{
		return;
	}
	Mood = InMood;
	IntenseTime = 0.0f;
	Switch(InMood == MusicMood::Silent ? std::string() : NextTrack(InMood));
}

void MusicDirector::Update(const float InDelta)
{
	if ( Mood == MusicMood::Intense )
	{
		IntenseTime += InDelta;
		if ( Pending != MusicMood::Intense && IntenseTime >= Settings.MinIntenseSeconds )
		{
			Request(Pending);
		}
	}
	const float Step = Settings.FadeSeconds > 0.0f ? InDelta / Settings.FadeSeconds : 1.0f;
	Current.Volume = StepTowards(Current.Volume, Current.Target, Step);
	Previous.Volume = StepTowards(Previous.Volume, Previous.Target, Step);
	if ( Previous.Volume <= 0.0f )
	{
		Previous = MusicVoice();
	}
}

MusicMood MusicDirector::GetMood() const
{
	return Mood;
}

const MusicVoice& MusicDirector::GetCurrent() const
{
	return Current;
}

const MusicVoice& MusicDirector::GetPrevious() const
{
	return Previous;
}

const MusicSettings& MusicDirector::GetSettings() const
{
	return Settings;
}

void MusicDirector::Switch(const std::string& InTrack)
{
	if ( InTrack == Current.Track && !InTrack.empty() )
	{
		Current.Target = 1.0f;
		return;
	}
	if ( !Current.Track.empty() )
	{
		Previous = Current;
		Previous.Target = 0.0f;
	}
	Current = MusicVoice();
	Current.Track = InTrack;
	Current.Target = InTrack.empty() ? 0.0f : 1.0f;
}

std::string MusicDirector::NextTrack(const MusicMood InMood)
{
	const TArray<std::string>& Tracks = InMood == MusicMood::Intense ? Settings.IntenseTracks
		: InMood == MusicMood::Menu ? Settings.MenuTracks : Settings.CalmTracks;
	if ( Tracks.IsEmpty() )
	{
		return std::string();
	}
	size_t& Next = InMood == MusicMood::Intense ? NextIntense : InMood == MusicMood::Menu ? NextMenu : NextCalm;
	const std::string Track = Tracks[Next % Tracks.Size()];
	Next = (Next + 1) % Tracks.Size();
	return Track;
}

} // namespace game::model

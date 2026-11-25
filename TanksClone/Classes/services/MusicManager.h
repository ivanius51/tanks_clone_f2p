#pragma once

#include <memory>
#include <string>
#include <vector>

#include "model/MusicDirector.h"
#include "templates/TUniquePtr.h"

namespace game
{
/**
 * Background music: loads the track lists from configs/audio/music.json, asks MusicDirector which tracks play
 * and how loud, and drives them through the audio port as looping sounds. Muted while sound is off.
 */
class MusicManager
{
public:
	MusicManager() = default;
	~MusicManager();
	MusicManager(const MusicManager&) = delete;
	MusicManager& operator=(const MusicManager&) = delete;

	void Init();
	void Request(model::MusicMood InMood, bool bInNewTrack = false);
	void PlayTrack(const std::string& InTrack);
	model::MusicMood GetMood() const;
	float GetIntenseProgress() const;

	void SetVolume(float InVolume);
	float GetVolume() const;
	void SetPaused(bool bInPaused);
	void Update(float InDelta);

	const std::string& GetCurrentTrack() const;

private:
	struct Playing
	{
		std::string Track;
		int Id = -1;
	};

	void Sync(const model::MusicVoice& InVoice, float InMaster);
	void StopAll();
	std::string TrackPath(const std::string& InTrackId) const;

	TUniquePtr<model::MusicDirector> Director;
	std::vector<Playing> Tracks;
	float Volume = 0.6f;
	float PausedVolume = 0.35f;
	float MenuVolume = 1.0f;
	std::vector<std::string> MenuTracks;
	bool bPaused = false;
};
} // namespace game

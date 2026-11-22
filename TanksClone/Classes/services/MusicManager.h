#pragma once

#include <memory>
#include <string>
#include <vector>

#include "model/MusicDirector.h"

namespace tanks
{

/**
 * Background music: loads the track lists from configs/music.json, asks MusicDirector which tracks play
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
	// Share of the level's enemies defeated after which a level turns intense.
	float GetIntenseProgress() const;

	// Music volume setting, 0..1.
	void SetVolume(float InVolume);
	float GetVolume() const;
	// Lowers the music to the configured share while the pause menu is open.
	void SetPaused(bool bInPaused);
	void Update(float InDelta);

	// The track playing at full volume now, empty when none; for tests.
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

	std::unique_ptr<model::MusicDirector> Director;
	std::vector<Playing> Tracks;
	float Volume = 0.6f;
	float PausedVolume = 0.35f;
	bool bPaused = false;
};

} // namespace tanks

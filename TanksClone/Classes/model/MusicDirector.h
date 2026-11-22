#pragma once

#include <string>
#include <vector>

namespace tanks::model
{

enum class MusicMood
{
	Silent,
	Menu,
	Calm,
	Intense
};

struct MusicSettings
{
	std::vector<std::string> MenuTracks;
	std::vector<std::string> CalmTracks;
	std::vector<std::string> IntenseTracks;
	float FadeSeconds = 1.5f;
	// An intense track plays at least this long before the music may calm down again.
	float MinIntenseSeconds = 12.0f;
	// Share of the level's enemies defeated after which the music turns intense.
	float IntenseProgress = 0.8f;
};

// One playing track and its fade.
struct MusicVoice
{
	std::string Track;
	float Volume = 0.0f;
	float Target = 0.0f;
};

/**
 * Picks the background track for the requested mood and crossfades between tracks. Pure logic:
 * the owner plays MusicVoice tracks at the given volumes (0..1, before the music volume setting).
 * Calm tracks rotate so the same one does not play twice in a row.
 */
class MusicDirector
{
public:
	explicit MusicDirector(const MusicSettings& InSettings);

	// InNewTrack: a new calm track even when calm music already plays (a new level).
	void Request(MusicMood InMood, bool bInNewTrack = false);
	void Update(float InDelta);

	MusicMood GetMood() const;
	const MusicVoice& GetCurrent() const;
	// The track fading out, empty when none.
	const MusicVoice& GetPrevious() const;
	const MusicSettings& GetSettings() const;

private:
	void Switch(const std::string& InTrack);
	std::string NextTrack(MusicMood InMood);

	MusicSettings Settings;
	MusicMood Mood = MusicMood::Silent;
	MusicMood Pending = MusicMood::Silent;
	MusicVoice Current;
	MusicVoice Previous;
	float IntenseTime = 0.0f;
	size_t NextMenu = 0;
	size_t NextCalm = 0;
	size_t NextIntense = 0;
};

} // namespace tanks::model

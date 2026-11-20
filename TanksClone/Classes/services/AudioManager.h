#pragma once

#include <vector>

#include "model/ResourceId.h"
#include "services/ResourceManager.h"

namespace tanks
{

// Plays sound effects. Does nothing while sound is disabled.
class AudioManager
{
public:
	AudioManager() = default;
	~AudioManager();
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	// Keeps the sounds of ids::sounds loaded so that the first shot does not stutter.
	void Preload();
	// InSound is an id from ids::sounds.
	void Play(const model::ResourceId& InSound);

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	// Volume of the sound effects, 0..1; it applies to the sounds played after the change.
	void SetVolume(float InVolume);
	float GetVolume() const;

private:
	std::vector<SoundHandle> Preloaded;
	bool bEnabled = true;
	float Volume = 1.0f;
};

} // namespace tanks

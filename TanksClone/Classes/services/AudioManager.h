#pragma once

#include "model/ResourceId.h"

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

	// Loads every sound of the resource table so that the first shot does not stutter.
	void Preload();
	// InSound is an id from ids::sounds.
	void Play(const model::ResourceId& InSound);

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	// Volume of the sound effects, 0..1.
	void SetVolume(float InVolume);
	float GetVolume() const;

private:
	bool bEnabled = true;
	float Volume = 1.0f;
};

} // namespace tanks

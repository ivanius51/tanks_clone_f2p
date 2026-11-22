#pragma once

#include <vector>

#include "model/ResourceId.h"
#include "ports/IAudioOutput.h"
#include "services/ResourceManager.h"

namespace tanks
{

// Plays sound effects. Does nothing while sound is disabled.
class AudioManager
{
public:
	AudioManager();
	~AudioManager();
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	// Keeps the sounds of ids::sounds loaded so that the first shot does not stutter.
	void Preload();
	// InSound is an id from ids::sounds.
	void Play(const model::ResourceId& InSound);

	// Replaces the audio port the sounds go to (the platform adapter by default); for tests.
	void SetOutput(ports::IAudioOutput* InOutput);

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	// Volume of the sound effects, 0..1; it applies to the sounds played after the change.
	void SetVolume(float InVolume);
	float GetVolume() const;

	// Output devices, the system default first; empty when the platform can't choose one.
	std::vector<std::string> GetDevices() const;
	// InName empty = system default.
	void SetDevice(const std::string& InName);
	std::string GetOpenedDevice() const;

private:
	ports::IAudioOutput* Output = nullptr;
	std::vector<SoundHandle> Preloaded;
	bool bEnabled = true;
	float Volume = 1.0f;
};

} // namespace tanks

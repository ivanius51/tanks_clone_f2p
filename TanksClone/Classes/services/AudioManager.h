#pragma once

#include <vector>

#include "model/ResourceId.h"
#include "ports/IAudioOutput.h"
#include "services/ResourceManager.h"

namespace tanks
{
/** Plays sound effects. Does nothing while sound is disabled. */
class AudioManager
{
public:
	AudioManager();
	~AudioManager();
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	void Preload();
	void Play(const model::ResourceId& InSound);

	void SetOutput(ports::IAudioOutput* InOutput);

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	void SetVolume(float InVolume);
	float GetVolume() const;

	std::vector<std::string> GetDevices() const;
	void SetDevice(const std::string& InName);
	std::string GetOpenedDevice() const;

private:
	ports::IAudioOutput* Output = nullptr;
	std::vector<SoundHandle> Preloaded;
	bool bEnabled = true;
	float Volume = 1.0f;
};
} // namespace tanks

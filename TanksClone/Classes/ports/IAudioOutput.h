#pragma once

#include <functional>
#include <string>
#include <vector>

namespace tanks::ports
{

/**
 * Port: plays sound effects. The game names a sound by the file path its resource id maps to; the adapter
 * (Cocos AudioEngine now, another engine later) decides how to load, cache and play it.
 */
class IAudioOutput
{
public:
	virtual ~IAudioOutput() = default;

	// Plays the sound once; Volume is 0..1.
	virtual void Play(const std::string& InPath, float InVolume) = 0;
	virtual void StopAll() = 0;
	// Loads the sound ahead of time. InDone runs on the main thread when it is ready (false on failure),
	// at once if the adapter cannot load in the background.
	virtual void Preload(const std::string& InPath, const std::function<void(bool)>& InDone) = 0;
	// Frees a preloaded sound.
	virtual void Unload(const std::string& InPath) = 0;

	// Starts a looping track (music); returns its id, or a negative id when it can't play.
	virtual int PlayLoop(const std::string& InPath, float InVolume) { return -1; }
	virtual void SetLoopVolume(int InId, float InVolume) {}
	virtual void StopLoop(int InId) {}

	// Output device names; the first is the system default. Empty when the adapter can't choose a device.
	virtual std::vector<std::string> GetDevices() const { return {}; }
	// Switches to the named device (empty = system default).
	virtual void SetDevice(const std::string& InName) {}
	// Name of the device the sound goes to now, empty when none is open.
	virtual std::string GetOpenedDevice() const { return {}; }
};

} // namespace tanks::ports

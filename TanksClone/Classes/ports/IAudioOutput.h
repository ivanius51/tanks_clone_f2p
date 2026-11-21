#pragma once

#include <functional>
#include <string>

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
};

} // namespace tanks::ports

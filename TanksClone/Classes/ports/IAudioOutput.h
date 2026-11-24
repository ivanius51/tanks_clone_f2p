#pragma once

#include <string>
#include <vector>

#include "templates/TFunction.h"

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

	virtual void Play(const std::string& InPath, float InVolume) = 0;
	virtual void StopAll() = 0;
	virtual void Preload(const std::string& InPath, const TFunction<void(bool)>& InDone) = 0;
	virtual void Unload(const std::string& InPath) = 0;

	virtual int PlayLoop(const std::string& InPath, float InVolume) { return -1; }
	virtual void SetLoopVolume(int InId, float InVolume) {}
	virtual void StopLoop(int InId) {}

	virtual std::vector<std::string> GetDevices() const { return {}; }
	virtual void SetDevice(const std::string& InName) {}
	virtual std::string GetOpenedDevice() const { return {}; }
};
} // namespace tanks::ports

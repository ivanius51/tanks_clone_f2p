#pragma once

#include "ports/IAudioOutput.h"

namespace tanks::view::cocos
{

/** IAudioOutput over cocos2d::experimental::AudioEngine. */
class CocosAudioOutput final : public ports::IAudioOutput
{
public:
	CocosAudioOutput() = default;
	~CocosAudioOutput() override;
	CocosAudioOutput(const CocosAudioOutput&) = delete;
	CocosAudioOutput& operator=(const CocosAudioOutput&) = delete;

	void Play(const std::string& InPath, float InVolume) override;
	void StopAll() override;
	void Preload(const std::string& InPath, const std::function<void(bool)>& InDone) override;
	void Unload(const std::string& InPath) override;
	std::vector<std::string> GetDevices() const override;
	void SetDevice(const std::string& InName) override;
	std::string GetOpenedDevice() const override;
};

} // namespace tanks::view::cocos

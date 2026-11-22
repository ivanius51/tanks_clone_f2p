#include "platform/cocos/CocosAudioOutput.h"

#include "audio/include/AudioEngine.h"

namespace tanks::view::cocos
{

CocosAudioOutput::~CocosAudioOutput()
{
	cocos2d::experimental::AudioEngine::end();
}

void CocosAudioOutput::Play(const std::string& InPath, const float InVolume)
{
	cocos2d::experimental::AudioEngine::play2d(InPath, false, InVolume);
}

void CocosAudioOutput::StopAll()
{
	cocos2d::experimental::AudioEngine::stopAll();
}

void CocosAudioOutput::Preload(const std::string& InPath, const std::function<void(bool)>& InDone)
{
	cocos2d::experimental::AudioEngine::preload(InPath, InDone ? InDone : [](bool) {});
}

void CocosAudioOutput::Unload(const std::string& InPath)
{
	cocos2d::experimental::AudioEngine::uncache(InPath);
}

std::vector<std::string> CocosAudioOutput::GetDevices() const
{
	return cocos2d::experimental::AudioEngine::getOutputDevices();
}

void CocosAudioOutput::SetDevice(const std::string& InName)
{
	cocos2d::experimental::AudioEngine::setOutputDevice(InName);
}

std::string CocosAudioOutput::GetOpenedDevice() const
{
	cocos2d::experimental::AudioEngine::lazyInit();
	return cocos2d::experimental::AudioEngine::getOpenedOutputDevice();
}

} // namespace tanks::view::cocos

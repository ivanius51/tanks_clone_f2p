#include "services/AudioManager.h"

#include <algorithm>
#include <string>

#include "audio/include/SimpleAudioEngine.h"

#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ResourceManager.h"

namespace tanks
{

namespace
{
	std::string SoundPath(const model::ResourceId& InSound)
	{
		return GameContext::Get().GetManager<ResourceManager>().ResolvePath(InSound, model::ResourceKind::Sound);
	}
}

void AudioManager::Preload()
{
	auto Engine = CocosDenshion::SimpleAudioEngine::getInstance();
	for ( const model::ResourceId& CurrentSound : ids::sounds::All )
	{
		const std::string Path = SoundPath(CurrentSound);
		if ( !Path.empty() )
		{
			Engine->preloadEffect(Path.c_str());
		}
	}
}

void AudioManager::Play(const model::ResourceId& InSound)
{
	if ( !bEnabled )
	{
		return;
	}
	const std::string Path = SoundPath(InSound);
	if ( !Path.empty() )
	{
		CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(Path.c_str());
	}
}

void AudioManager::SetEnabled(const bool bInEnabled)
{
	bEnabled = bInEnabled;
	if ( !bEnabled )
	{
		CocosDenshion::SimpleAudioEngine::getInstance()->stopAllEffects();
	}
}

bool AudioManager::IsEnabled() const
{
	return bEnabled;
}

void AudioManager::SetVolume(const float InVolume)
{
	Volume = std::max(0.0f, std::min(1.0f, InVolume));
	CocosDenshion::SimpleAudioEngine::getInstance()->setEffectsVolume(Volume);
}

float AudioManager::GetVolume() const
{
	return Volume;
}

AudioManager::~AudioManager()
{
	CocosDenshion::SimpleAudioEngine::end();
}

} // namespace tanks

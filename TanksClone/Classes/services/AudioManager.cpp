#include "services/AudioManager.h"

#include <algorithm>
#include <string>

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

AudioManager::AudioManager() : Output(&GameContext::Get().GetAudioOutput())
{
}

void AudioManager::SetOutput(ports::IAudioOutput* const InOutput)
{
	Output = InOutput ? InOutput : &GameContext::Get().GetAudioOutput();
}

void AudioManager::Preload()
{
	for ( const model::ResourceId& CurrentSound : ids::sounds::All )
	{
		if ( SoundHandle Handle = GameContext::Get().GetManager<ResourceManager>().AcquireSound(CurrentSound.GetName()) )
		{
			Preloaded.push_back(std::move(Handle));
		}
	}
}

void AudioManager::Play(const model::ResourceId& InSound)
{
	if ( !bEnabled || !InSound.IsValid() )
	{
		return;
	}
	const std::string Path = SoundPath(InSound);
	if ( !Path.empty() )
	{
		Output->Play(Path, Volume);
	}
}

void AudioManager::SetEnabled(const bool bInEnabled)
{
	bEnabled = bInEnabled;
	if ( !bEnabled )
	{
		Output->StopAll();
	}
}

bool AudioManager::IsEnabled() const
{
	return bEnabled;
}

void AudioManager::SetVolume(const float InVolume)
{
	Volume = std::max(0.0f, std::min(1.0f, InVolume));
}

float AudioManager::GetVolume() const
{
	return Volume;
}

std::vector<std::string> AudioManager::GetDevices() const
{
	return Output->GetDevices();
}

void AudioManager::SetDevice(const std::string& InName)
{
	Output->SetDevice(InName);
}

std::string AudioManager::GetOpenedDevice() const
{
	return Output->GetOpenedDevice();
}

AudioManager::~AudioManager()
{
	Preloaded.clear();
}

} // namespace tanks

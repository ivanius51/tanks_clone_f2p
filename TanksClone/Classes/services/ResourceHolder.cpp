#include "services/ResourceHolder.h"

#include <algorithm>

#include "core/GameContext.h"
#include "templates/TContainers.h"

namespace game
{

ResourceHolder::ResourceHolder(const TArray<std::string>& InAtlases)
{
	for ( const std::string& CurrentAtlas : InAtlases )
	{
		AddAtlas(CurrentAtlas);
	}
}

bool ResourceHolder::AddAtlas(const std::string& InAtlas)
{
	if ( std::find(AtlasNames.begin(), AtlasNames.end(), InAtlas) != AtlasNames.end() )
	{
		return true;
	}
	AtlasHandle Handle = GameContext::Get<ResourceManager>().AcquireAtlas(InAtlas);
	if ( !Handle )
	{
		return false;
	}
	AtlasHandles.PushBack(std::move(Handle));
	AtlasNames.PushBack(InAtlas);
	return true;
}

void ResourceHolder::AddAtlas(const std::string& InAtlas, AtlasHandle InHandle)
{
	if ( InHandle && std::find(AtlasNames.begin(), AtlasNames.end(), InAtlas) == AtlasNames.end() )
	{
		AtlasHandles.PushBack(std::move(InHandle));
		AtlasNames.PushBack(InAtlas);
	}
}

void ResourceHolder::AddSound(const std::string& InSound, SoundHandle InHandle)
{
	if ( InHandle && std::find(SoundNames.begin(), SoundNames.end(), InSound) == SoundNames.end() )
	{
		SoundHandles.PushBack(std::move(InHandle));
		SoundNames.PushBack(InSound);
	}
}

bool ResourceHolder::AddSound(const std::string& InSound)
{
	if ( std::find(SoundNames.begin(), SoundNames.end(), InSound) != SoundNames.end() )
	{
		return true;
	}
	SoundHandle Handle = GameContext::Get<ResourceManager>().AcquireSound(InSound);
	if ( !Handle )
	{
		return false;
	}
	SoundHandles.PushBack(std::move(Handle));
	SoundNames.PushBack(InSound);
	return true;
}

const TArray<std::string>& ResourceHolder::GetAtlases() const
{
	return AtlasNames;
}

const TArray<std::string>& ResourceHolder::GetSounds() const
{
	return SoundNames;
}

} // namespace game

#include "services/ResourceHolder.h"

#include <algorithm>

#include "core/GameContext.h"

namespace tanks
{

ResourceHolder::ResourceHolder(const std::vector<std::string>& InAtlases)
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
	AtlasHandle Handle = GameContext::Get().GetManager<ResourceManager>().AcquireAtlas(InAtlas);
	if ( !Handle )
	{
		return false;
	}
	AtlasHandles.push_back(std::move(Handle));
	AtlasNames.push_back(InAtlas);
	return true;
}

void ResourceHolder::AddAtlas(const std::string& InAtlas, AtlasHandle InHandle)
{
	if ( InHandle && std::find(AtlasNames.begin(), AtlasNames.end(), InAtlas) == AtlasNames.end() )
	{
		AtlasHandles.push_back(std::move(InHandle));
		AtlasNames.push_back(InAtlas);
	}
}

void ResourceHolder::AddSound(const std::string& InSound, SoundHandle InHandle)
{
	if ( InHandle && std::find(SoundNames.begin(), SoundNames.end(), InSound) == SoundNames.end() )
	{
		SoundHandles.push_back(std::move(InHandle));
		SoundNames.push_back(InSound);
	}
}

bool ResourceHolder::AddSound(const std::string& InSound)
{
	if ( std::find(SoundNames.begin(), SoundNames.end(), InSound) != SoundNames.end() )
	{
		return true;
	}
	SoundHandle Handle = GameContext::Get().GetManager<ResourceManager>().AcquireSound(InSound);
	if ( !Handle )
	{
		return false;
	}
	SoundHandles.push_back(std::move(Handle));
	SoundNames.push_back(InSound);
	return true;
}

const std::vector<std::string>& ResourceHolder::GetAtlases() const
{
	return AtlasNames;
}

const std::vector<std::string>& ResourceHolder::GetSounds() const
{
	return SoundNames;
}

} // namespace tanks

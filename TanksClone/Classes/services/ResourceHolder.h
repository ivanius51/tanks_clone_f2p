#pragma once

#include <string>

#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game
{
/**
 * Holds handles to loaded atlases and sounds (by id name), e.g. everything one level needs: they stay loaded
 * while the holder lives and unload with it, unless another holder still needs them.
 */
class ResourceHolder
{
public:
	ResourceHolder() = default;
	explicit ResourceHolder(const TArray<std::string>& InAtlases);

	bool AddAtlas(const std::string& InAtlas);
	void AddAtlas(const std::string& InAtlas, AtlasHandle InHandle);
	bool AddSound(const std::string& InSound);
	void AddSound(const std::string& InSound, SoundHandle InHandle);

	const TArray<std::string>& GetAtlases() const;
	const TArray<std::string>& GetSounds() const;

private:
	TArray<AtlasHandle> AtlasHandles;
	TArray<SoundHandle> SoundHandles;
	TArray<std::string> AtlasNames;
	TArray<std::string> SoundNames;
};
} // namespace game

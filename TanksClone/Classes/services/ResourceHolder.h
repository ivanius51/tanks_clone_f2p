#pragma once

#include <string>
#include <vector>

#include "services/ResourceManager.h"

namespace tanks
{

/**
 * Holds handles to loaded atlases and sounds (by id name), e.g. everything one level needs: they stay loaded
 * while the holder lives and unload with it, unless another holder still needs them.
 */
class ResourceHolder
{
public:
	ResourceHolder() = default;
	explicit ResourceHolder(const std::vector<std::string>& InAtlases);

	// Return false if the resource could not be loaded.
	bool AddAtlas(const std::string& InAtlas);
	// Adds an atlas that is already loaded (a handle from the manager).
	void AddAtlas(const std::string& InAtlas, AtlasHandle InHandle);
	bool AddSound(const std::string& InSound);

	const std::vector<std::string>& GetAtlases() const;
	const std::vector<std::string>& GetSounds() const;

private:
	std::vector<AtlasHandle> AtlasHandles;
	std::vector<SoundHandle> SoundHandles;
	std::vector<std::string> AtlasNames;
	std::vector<std::string> SoundNames;
};

} // namespace tanks

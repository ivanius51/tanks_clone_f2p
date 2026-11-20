#pragma once

#include <string>
#include <vector>

#include "services/ResourceManager.h"

namespace tanks
{

/**
 * Holds handles to a group of atlases (by atlas id name), e.g. the atlases of one level: they stay loaded
 * while the set lives and unload with it, unless another holder still needs them.
 */
class AtlasSet
{
public:
	AtlasSet() = default;
	explicit AtlasSet(const std::vector<std::string>& InAtlases);

	// Returns false if the atlas could not be loaded.
	bool Add(const std::string& InAtlas);
	const std::vector<std::string>& GetAtlases() const;

private:
	std::vector<AtlasHandle> Handles;
	std::vector<std::string> Names;
};

} // namespace tanks

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace tanks
{
/** Names (resource ids) of what a screen needs loaded, e.g. one level. */
struct ResourceSet
{
	std::vector<std::string> Atlases;
	std::vector<std::string> Sounds;

	std::size_t GetCount() const
	{
		return Atlases.size() + Sounds.size();
	}
};
} // namespace tanks

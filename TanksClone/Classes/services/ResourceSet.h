#pragma once

#include <cstddef>
#include <string>
#include "templates/TContainers.h"

namespace game
{
/** Names (resource ids) of what a screen needs loaded, e.g. one level. */
struct ResourceSet
{
	TArray<std::string> Atlases;
	TArray<std::string> Sounds;

	std::size_t GetCount() const
	{
		return Atlases.Size() + Sounds.Size();
	}
};
} // namespace game

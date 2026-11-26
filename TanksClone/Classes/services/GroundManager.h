#pragma once

#include "model/GroundMap.h"

namespace game
{
/** Holds the floor effects (roads, mud) of the level being played; the scene sets them at level start, tanks read them while driving. */
class GroundManager
{
public:
	void Set(model::GroundMap InMap, const float InTileSize)
	{
		Map = std::move(InMap);
		TileSize = InTileSize;
	}
	void Clear() { Map.Clear(); }

	model::GroundEffect GetEffectAt(const float InX, const float InY) const { return Map.GetEffectAtWorld(InX, InY, TileSize); }

private:
	model::GroundMap Map;
	float TileSize = 1.0f;
};
} // namespace game

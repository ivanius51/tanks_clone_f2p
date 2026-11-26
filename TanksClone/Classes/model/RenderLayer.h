#pragma once

#include <array>
#include <string_view>

#include "model/ConfigNode.h"

namespace game::model
{
/** Named draw layers of the play field, from the back to the front. The order values are data (configs/balance/render_layers.json). */
enum class RenderLayer
{
	Floor,
	Ground,
	GroundShadow,
	Walls,
	Debris,
	Pickups,
	Tanks,
	Bullets,
	HighBullets,
	WallShadows,
	Effects,
	EffectsTop,
	Count
};

constexpr int RenderLayerCount = static_cast<int>(RenderLayer::Count);

/** The config key of a layer. */
std::string_view GetRenderLayerKey(RenderLayer InLayer);

/** Finds a layer by its config key; false when the key is unknown. */
bool FindRenderLayer(std::string_view InKey, RenderLayer& OutLayer);

/** The draw order of every named layer; a view adds its own offset inside a layer. */
class RenderLayers
{
public:
	RenderLayers();

	void Load(const IConfigNode& InNode);
	int GetOrder(RenderLayer InLayer) const;

private:
	std::array<int, RenderLayerCount> Orders;
};
} // namespace game::model

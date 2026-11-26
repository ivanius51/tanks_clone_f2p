#include "model/RenderLayer.h"

namespace game::model
{
namespace
{
constexpr std::array<std::string_view, RenderLayerCount> Keys = { "floor", "ground", "ground_shadow", "walls", "debris", "pickups", "tanks", "bullets", "high_bullets", "wall_shadows", "effects", "effects_top" };
constexpr std::array<int, RenderLayerCount> FallbackOrders = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
}

std::string_view GetRenderLayerKey(const RenderLayer InLayer)
{
	return Keys[static_cast<size_t>(InLayer)];
}

bool FindRenderLayer(const std::string_view InKey, RenderLayer& OutLayer)
{
	for ( size_t Index = 0; Index < Keys.size(); ++Index )
	{
		if ( Keys[Index] == InKey )
		{
			OutLayer = static_cast<RenderLayer>(Index);
			return true;
		}
	}
	return false;
}

RenderLayers::RenderLayers()
	: Orders{ FallbackOrders }
{
}

void RenderLayers::Load(const IConfigNode& InNode)
{
	for ( size_t Index = 0; Index < Orders.size(); ++Index )
	{
		Orders[Index] = InNode.GetValueOr(Keys[Index], Orders[Index]);
	}
}

int RenderLayers::GetOrder(const RenderLayer InLayer) const
{
	return Orders[static_cast<size_t>(InLayer)];
}
} // namespace game::model

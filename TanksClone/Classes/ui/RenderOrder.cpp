#include "ui/RenderOrder.h"

#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"

namespace game::view::cocos
{
namespace
{
const model::RenderLayers& GetLayers()
{
	static const model::RenderLayers GLayers = []()
	{
		model::RenderLayers Result;
		if ( const model::IConfigNode* const Node = config_refs::FindChild(ids::configs::RenderLayers, "render_layers") )
		{
			Result.Load(*Node);
		}
		return Result;
	}();
	return GLayers;
}
}

int GetZOrder(const model::RenderLayer InLayer, const int InOffset)
{
	return GetLayers().GetOrder(InLayer) + InOffset;
}
} // namespace game::view::cocos

#pragma once

#include <string>
#include <string_view>

#include "game/EnemyCatalog.h"
#include "model/ConfigNode.h"

namespace tanks
{
/** The only place that knows the layout of an enemy config; the catalog gets plain EnemyDef values. */
class EnemyCatalogLoader
{
public:
	static EnemyDef ReadEnemy(const model::IConfigNode& InNode, std::string_view InConfigId);
	static std::string IdFromConfigId(std::string_view InConfigId);
};
} // namespace tanks

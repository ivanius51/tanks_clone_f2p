#pragma once

#include "core/ObjectEnums.h"
#include "game/ai/AiSettings.h"
#include "model/ConfigNode.h"

namespace game
{
/** The only place that knows the layout of the AI configs; game code gets plain settings. */
class AiSettingsLoader
{
public:
	static TankAiSettings LoadTankAi(EAiType InType);
	static PlayerBotSettings LoadPlayerBot();

	static TankAiSettings ReadTankAi(const model::IConfigNode& InNode);
	static PlayerBotSettings ReadPlayerBot(const model::IConfigNode& InNode);
};
} // namespace game

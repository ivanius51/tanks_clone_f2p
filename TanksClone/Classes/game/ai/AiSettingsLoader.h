#pragma once

#include "core/ObjectEnums.h"
#include "game/ai/AiSettings.h"
#include "model/ConfigNode.h"

namespace tanks
{

/** The only place that knows the layout of the AI configs; game code gets plain settings. */
class AiSettingsLoader
{
public:
	// configs/ai/tank_ai.json, block "profiles" / <profile of InType>.
	static TankAiSettings LoadTankAi(AiType InType);
	// configs/ai/player_bot.json.
	static PlayerBotSettings LoadPlayerBot();

	// Missing keys are reported by the node and read as 0.
	static TankAiSettings ReadTankAi(const model::IConfigNode& InNode);
	static PlayerBotSettings ReadPlayerBot(const model::IConfigNode& InNode);
};

} // namespace tanks

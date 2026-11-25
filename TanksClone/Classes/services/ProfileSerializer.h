#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "services/ProfileManager.h"

namespace game
{
/** Converts the player profile to and from JSON text; knows nothing about where the text is kept. */
class ProfileSerializer
{
public:
	static std::string ToJson(const PlayerProfile& InProfile);
	static PlayerProfile FromJson(const model::IConfigNode& InNode);
};
} // namespace game

#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "services/ProfileManager.h"

namespace tanks
{

/** Converts the player profile to and from JSON text; knows nothing about where the text is kept. */
class ProfileSerializer
{
public:
	static std::string ToJson(const PlayerProfile& InProfile);
	// Missing keys keep the defaults of a fresh profile.
	static PlayerProfile FromJson(const model::IConfigNode& InNode);
};

} // namespace tanks

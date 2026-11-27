#pragma once

#include <string>
#include <string_view>

#include "cocos2d.h"

#include "templates/TContainers.h"

namespace game
{
class Tank;
namespace model
{
class IConfigNode;
}

/** An effect a state replays on its tank every Interval seconds, hitting the tank from Shot (a direction; zero for none). */
struct TankStateEffect
{
	std::string Effect;
	float Interval = 0.7f;
	cocos2d::Vec2 Shot;
};

/** How a tank is shown: health share, hits from the sides, a permanent buff, a forced AI tactic and scripted effects. */
struct TankState
{
	std::string Label;
	float HealthShare = 1.0f;
	TArray<cocos2d::Vec2> Hits;
	std::string Buff;
	std::string Tactic;
	TArray<TankStateEffect> Effects;
};

/**
 * Tank looks written in data (configs/views/tank_states.json): named states and the damage stages that pick one by health
 * share. A state node may name another with "state" and then override its fields. The debug galleries and the menu
 * garage show tanks through Apply, so a look is made once and used in both.
 */
class TankStateScript
{
public:
	static TankState FromNode(const model::IConfigNode& InNode);
	static bool FindNamed(std::string_view InId, TankState& OutState);
	// The state of the damage stage that covers InHealthShare (1 is untouched).
	static TankState ForHealthShare(float InHealthShare);
	static void Apply(Tank& InTank, const TankState& InState);
};

} // namespace game

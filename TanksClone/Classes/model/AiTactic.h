#pragma once

#include <string_view>

namespace tanks::model
{

/** What an enemy is doing right now; shown over the tank by the AI debug view. */
enum class AiTactic
{
	Wander,
	Chase,
	Flank,
	Pin,
	Retreat,
	GuardDrop,
	AttackBase,
	HuntCamper,
	Dodge,
	Cover,
	FakeTurn,
	Count
};

std::string_view GetTacticName(AiTactic InTactic);

} // namespace tanks::model

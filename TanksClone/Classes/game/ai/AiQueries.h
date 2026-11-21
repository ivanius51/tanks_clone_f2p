#pragma once

#include "cocos2d.h"

#include "core/ObjectEnums.h"

namespace tanks
{

class Tank;
class Wall;

/** What the AI asks about the world: where the player and the enemies are, and which way to try next. */
class AiQueries
{
public:
	static Tank* FindPlayer();
	// The closest live enemy tank that is on the map, or nullptr.
	static Tank* FindNearestEnemy(const cocos2d::Vec2& InFrom);
	// The base of the player if the level has one standing, or nullptr.
	static Wall* FindBase();
	// One of MoveForward, MoveBack, MoveLeft, MoveRight, chosen at random.
	static CommandType RandomMove();
	// The move command that heads along the dominant axis of InDelta.
	static CommandType MoveAlongMajorAxis(const cocos2d::Vec2& InDelta);
	// The move command that heads along the shorter axis of InDelta.
	static CommandType MoveAlongMinorAxis(const cocos2d::Vec2& InDelta);
};

} // namespace tanks

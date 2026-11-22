#pragma once

#include "cocos2d.h"

#include "core/ObjectEnums.h"

namespace tanks
{

class GameObject;
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
	// The nearest thing straight ahead of InOwner within InRange that blocks or threatens its way and can be shot apart
	// (a brick wall, a wreck, a barrel, a mine); nullptr when the nearest one is unbreakable or nothing is there.
	static GameObject* FindBreakableAhead(const Tank& InOwner, float InRange);
	// The first move along the cheapest way over the map cells to InTarget; Undefined when there is no way.
	static CommandType MoveAlongPath(const Tank& InOwner, const cocos2d::Vec2& InTarget, int InBreakCost);
	// One of MoveForward, MoveBack, MoveLeft, MoveRight, chosen at random.
	static CommandType RandomMove();
	// The move command that heads along the dominant axis of InDelta.
	static CommandType MoveAlongMajorAxis(const cocos2d::Vec2& InDelta);
	// The move command that heads along the shorter axis of InDelta.
	static CommandType MoveAlongMinorAxis(const cocos2d::Vec2& InDelta);
};

} // namespace tanks

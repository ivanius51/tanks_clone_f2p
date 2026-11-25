#pragma once

#include "cocos2d.h"

#include "core/ObjectEnums.h"

namespace game
{
class GameObject;
class Tank;
class Wall;

/** What the AI asks about the world: where the player and the enemies are, and which way to try next. */
class AiQueries
{
public:
	static Tank* FindPlayer();
	static Tank* FindNearestEnemy(const cocos2d::Vec2& InFrom);
	static Wall* FindBase();
	static GameObject* FindBreakableAhead(const Tank& InOwner, float InRange);
	static CommandType MoveAlongPath(const Tank& InOwner, const cocos2d::Vec2& InTarget, int InBreakCost);
	static CommandType RandomMove();
	static CommandType MoveAlongMajorAxis(const cocos2d::Vec2& InDelta);
	static CommandType MoveAlongMinorAxis(const cocos2d::Vec2& InDelta);
};
} // namespace game

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
	// The player as seen by InObserver: a player hiding in cover is seen only from close up, or after shooting.
	static Tank* FindVisiblePlayer(const Tank& InObserver);
	static Tank* FindNearestEnemy(const cocos2d::Vec2& InFrom);
	static Wall* FindBase();
	static GameObject* FindBreakableAhead(const Tank& InOwner, float InRange);
	static ECommandType MoveAlongPath(const Tank& InOwner, const cocos2d::Vec2& InTarget, int InBreakCost);
	static ECommandType RandomMove();
	static ECommandType MoveAlongMajorAxis(const cocos2d::Vec2& InDelta);
	static ECommandType MoveAlongMinorAxis(const cocos2d::Vec2& InDelta);
};
} // namespace game

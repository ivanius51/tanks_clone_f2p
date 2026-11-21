#include "game/ai/AiQueries.h"

#include <cmath>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "model/Random.h"

namespace tanks
{

namespace
{
	static_assert(static_cast<int>(CommandType::MoveBack) == static_cast<int>(CommandType::MoveForward) + 1
		&& static_cast<int>(CommandType::MoveLeft) == static_cast<int>(CommandType::MoveForward) + 2
		&& static_cast<int>(CommandType::MoveRight) == static_cast<int>(CommandType::MoveForward) + 3,
		"RandomMove picks from the contiguous range MoveForward..MoveRight");

	CommandType MoveHorizontal(const float InDeltaX)
	{
		return InDeltaX > 0.0f ? CommandType::MoveRight : CommandType::MoveLeft;
	}

	CommandType MoveVertical(const float InDeltaY)
	{
		return InDeltaY > 0.0f ? CommandType::MoveForward : CommandType::MoveBack;
	}
}

Tank* AiQueries::FindPlayer()
{
	return GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName);
}

Wall* AiQueries::FindBase()
{
	Wall* Found = nullptr;
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Wall>([&Found](Wall& InWall)
	{
		if ( !Found && InWall.IsBase() && !InWall.IsDestroying() && InWall.GetVisualNode() )
		{
			Found = &InWall;
		}
	});
	return Found;
}

Tank* AiQueries::FindNearestEnemy(const cocos2d::Vec2& InFrom)
{
	Tank* Nearest = nullptr;
	float BestDistance = 0.0f;
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
	{
		if ( InTank.GetTeam() != Team::Enemy || InTank.IsDestroying() || !InTank.GetVisualNode() )
		{
			return;
		}
		const float Distance = InTank.GetVisualNode()->getPosition().distance(InFrom);
		if ( !Nearest || Distance < BestDistance )
		{
			Nearest = &InTank;
			BestDistance = Distance;
		}
	});
	return Nearest;
}

CommandType AiQueries::RandomMove()
{
	const int First = static_cast<int>(CommandType::MoveForward);
	const int Last = static_cast<int>(CommandType::MoveRight);
	return static_cast<CommandType>(model::GameRandom().Int(First, Last));
}

CommandType AiQueries::MoveAlongMajorAxis(const cocos2d::Vec2& InDelta)
{
	return std::fabs(InDelta.x) > std::fabs(InDelta.y) ? MoveHorizontal(InDelta.x) : MoveVertical(InDelta.y);
}

CommandType AiQueries::MoveAlongMinorAxis(const cocos2d::Vec2& InDelta)
{
	return std::fabs(InDelta.x) < std::fabs(InDelta.y) ? MoveHorizontal(InDelta.x) : MoveVertical(InDelta.y);
}

} // namespace tanks

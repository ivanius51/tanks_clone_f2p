#include "game/ai/AiQueries.h"

#include <cmath>
#include <vector>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/objects/Bullet.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "model/GridPath.h"
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

GameObject* AiQueries::FindBreakableAhead(const Tank& InOwner, const float InRange)
{
	const cocos2d::Node* const OwnerNode = InOwner.GetVisualNode();
	if ( !OwnerNode )
	{
		return nullptr;
	}
	const cocos2d::Vec2 From = OwnerNode->getPosition();
	const cocos2d::Vec2& Direction = InOwner.GetDirection();
	const float HalfWidth = TileSize * 0.4f;
	GameObject* Nearest = nullptr;
	float NearestDistance = InRange;
	GameContext::Get().GetManager<ObjectsFactory>().FindObjectIf([&](GameObject& InObject)
	{
		const cocos2d::Node* const Node = InObject.GetVisualNode();
		if ( !Node || InObject.IsDestroying() || InObject.GetType() == ObjectType::Vehicle || InObject.GetTeam() == Team::Enemy )
		{
			return false;
		}
		const bool bMine = InObject.GetType() == ObjectType::Bullet && ObjectCast<Bullet>(&InObject)->GetVelocity().isZero();
		const bool bBlocker = InObject.GetType() == ObjectType::Wall && !InObject.IsPassable();
		if ( !bMine && !bBlocker )
		{
			return false;
		}
		const cocos2d::Vec2 Delta = Node->getPosition() - From;
		const float Along = Delta.dot(Direction);
		if ( Along > 0.0f && Along < NearestDistance && std::fabs(Delta.cross(Direction)) < HalfWidth )
		{
			NearestDistance = Along;
			Nearest = &InObject;
		}
		return false;
	});
	return Nearest && Nearest->IsDestroyable() ? Nearest : nullptr;
}

CommandType AiQueries::MoveAlongPath(const Tank& InOwner, const cocos2d::Vec2& InTarget, const int InBreakCost)
{
	const cocos2d::Node* const OwnerNode = InOwner.GetVisualNode();
	if ( !OwnerNode )
	{
		return CommandType::Undefined;
	}
	const auto CellOf = [](const cocos2d::Vec2& InPoint)
	{
		return model::GridCell{ static_cast<int>(std::floor(InPoint.x / TileSize)), static_cast<int>(std::floor(InPoint.y / TileSize)) };
	};
	const cocos2d::Rect& Bounds = InOwner.GetBounds();
	model::GridPath Grid(static_cast<int>(Bounds.size.width / TileSize), static_cast<int>(Bounds.size.height / TileSize));
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject(ObjectType::Wall, [&Grid, &CellOf](GameObject& InObject)
	{
		if ( InObject.GetVisualNode() && !InObject.IsDestroying() && !InObject.IsPassable() )
		{
			Grid.Set(CellOf(InObject.GetVisualNode()->getPosition()), InObject.IsDestroyable() ? model::CellPass::Breakable : model::CellPass::Blocked);
		}
	});
	const model::GridCell From = CellOf(OwnerNode->getPosition());
	const std::vector<model::GridCell> Path = Grid.Find(From, CellOf(InTarget), InBreakCost);
	if ( Path.empty() )
	{
		return CommandType::Undefined;
	}
	return MoveAlongMajorAxis(cocos2d::Vec2(static_cast<float>(Path.front().X - From.X), static_cast<float>(Path.front().Y - From.Y)));
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

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

namespace game
{

namespace
{
	static_assert(static_cast<int>(ECommandType::MoveBack) == static_cast<int>(ECommandType::MoveForward) + 1
		&& static_cast<int>(ECommandType::MoveLeft) == static_cast<int>(ECommandType::MoveForward) + 2
		&& static_cast<int>(ECommandType::MoveRight) == static_cast<int>(ECommandType::MoveForward) + 3,
		"RandomMove picks from the contiguous range MoveForward..MoveRight");

	ECommandType MoveHorizontal(const float InDeltaX)
	{
		return InDeltaX > 0.0f ? ECommandType::MoveRight : ECommandType::MoveLeft;
	}

	ECommandType MoveVertical(const float InDeltaY)
	{
		return InDeltaY > 0.0f ? ECommandType::MoveForward : ECommandType::MoveBack;
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
	GameContext::Get<ObjectsFactory>().FindObjectIf([&](const FGameObjectHandle& InObject)
	{
		const cocos2d::Node* const Node = InObject->GetVisualNode();
		if ( !Node || InObject->IsDestroying() || InObject->GetType() == EObjectType::Vehicle || InObject->GetTeam() == ETeam::Enemy )
		{
			return false;
		}
		const bool bMine = InObject->GetType() == EObjectType::Bullet && ObjectCast<Bullet>(InObject.Get())->GetVelocity().isZero();
		const bool bBlocker = InObject->GetType() == EObjectType::Wall && !InObject->IsPassable();
		if ( !bMine && !bBlocker )
		{
			return false;
		}
		const cocos2d::Vec2 Delta = Node->getPosition() - From;
		const float Along = Delta.dot(Direction);
		if ( Along > 0.0f && Along < NearestDistance && std::fabs(Delta.cross(Direction)) < HalfWidth )
		{
			NearestDistance = Along;
			Nearest = InObject.Get();
		}
		return false;
	});
	return Nearest && Nearest->IsDestroyable() ? Nearest : nullptr;
}

ECommandType AiQueries::MoveAlongPath(const Tank& InOwner, const cocos2d::Vec2& InTarget, const int InBreakCost)
{
	const cocos2d::Node* const OwnerNode = InOwner.GetVisualNode();
	if ( !OwnerNode )
	{
		return ECommandType::Undefined;
	}
	const auto CellOf = [](const cocos2d::Vec2& InPoint)
	{
		return model::GridCell{ static_cast<int>(std::floor(InPoint.x / TileSize)), static_cast<int>(std::floor(InPoint.y / TileSize)) };
	};
	const cocos2d::Rect& Bounds = InOwner.GetBounds();
	model::GridPath Grid(static_cast<int>(Bounds.size.width / TileSize), static_cast<int>(Bounds.size.height / TileSize));
	GameContext::Get<ObjectsFactory>().ForEachObject<Wall>([&Grid, &CellOf](Wall& InObject)
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
		return ECommandType::Undefined;
	}
	return MoveAlongMajorAxis(cocos2d::Vec2(static_cast<float>(Path.front().X - From.X), static_cast<float>(Path.front().Y - From.Y)));
}

Tank* AiQueries::FindPlayer()
{
	return GameContext::Get<ObjectsFactory>().FindObject<Tank>(PlayerName).Get();
}

Wall* AiQueries::FindBase()
{
	FWallHandle Found = GameContext::Get<ObjectsFactory>().FindObjectIf<Wall>([](Wall& InObject)
	{
		if ( InObject.IsDestroying() || !InObject.GetVisualNode() || !InObject.IsBase() )
		{
			return false;
		}
		return true;
	});

	return Found.Get();
}

Tank* AiQueries::FindNearestEnemy(const cocos2d::Vec2& InFrom)
{
	Tank* Nearest = nullptr;
	float BestDistance = 0.0f;
	GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
	{
		if ( InTank.GetTeam() != ETeam::Enemy || InTank.IsDestroying() || !InTank.GetVisualNode() )
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

ECommandType AiQueries::RandomMove()
{
	const int First = static_cast<int>(ECommandType::MoveForward);
	const int Last = static_cast<int>(ECommandType::MoveRight);
	return static_cast<ECommandType>(model::GameRandom().Int(First, Last));
}

ECommandType AiQueries::MoveAlongMajorAxis(const cocos2d::Vec2& InDelta)
{
	return std::fabs(InDelta.x) > std::fabs(InDelta.y) ? MoveHorizontal(InDelta.x) : MoveVertical(InDelta.y);
}

ECommandType AiQueries::MoveAlongMinorAxis(const cocos2d::Vec2& InDelta)
{
	return std::fabs(InDelta.x) < std::fabs(InDelta.y) ? MoveHorizontal(InDelta.x) : MoveVertical(InDelta.y);
}

} // namespace game

#include "game/objects/PhysicsObject.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "core/GameContext.h"
#include "game/ObjectsFactory.h"

namespace game
{

namespace
{
	constexpr float ParallelEpsilon = 1e-6f;
	constexpr float OverlapEpsilon = 0.01f;

	float OverlapArea(const cocos2d::Rect& InFirst, const cocos2d::Rect& InSecond)
	{
		const float CurrentWidth = std::min(InFirst.getMaxX(), InSecond.getMaxX()) - std::max(InFirst.getMinX(), InSecond.getMinX());
		const float CurrentHeight = std::min(InFirst.getMaxY(), InSecond.getMaxY()) - std::max(InFirst.getMinY(), InSecond.getMinY());
		return CurrentWidth > 0.0f && CurrentHeight > 0.0f ? CurrentWidth * CurrentHeight : 0.0f;
	}
}

bool PhysicsObject::Build()
{
	SetPassable(false);
	return true;
}

void PhysicsObject::SetBounds(const cocos2d::Rect& InBounds)
{
	Bounds = InBounds;
	bHasBounds = true;
}

const cocos2d::Rect& PhysicsObject::GetBounds() const
{
	return Bounds;
}

bool PhysicsObject::HasBounds() const
{
	return bHasBounds;
}

cocos2d::Rect PhysicsObject::GetCollisionRect() const
{
	auto CurrentVisualNode = GetVisualNode();
	return CurrentVisualNode ? GetCollisionRectAt(CurrentVisualNode->getPosition()) : cocos2d::Rect();
}

cocos2d::Rect PhysicsObject::GetCollisionRectAt(const cocos2d::Vec2& InPosition) const
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return cocos2d::Rect();
	}

	// Size of the visible sprite pixels, scaled.
	cocos2d::Size CurrentSize = CurrentVisualNode->getContentSize();
	if ( auto CurrentSprite = CheckedCast<cocos2d::Sprite>(CurrentVisualNode) )
	{
		if ( auto CurrentFrame = CurrentSprite->getSpriteFrame() )
		{
			CurrentSize = CurrentFrame->getRect().size;
		}
	}
	CurrentSize.width *= CurrentVisualNode->getScaleX();
	CurrentSize.height *= CurrentVisualNode->getScaleY();

	// A vehicle takes the same room at any heading, so turning never pushes it into a wall.
	if ( GetType() == EObjectType::Vehicle )
	{
		CurrentSize.width = CurrentSize.height = std::max(CurrentSize.width, CurrentSize.height);
	}

	// A quarter turn swaps the sides.
	const float Angle = std::fabs(std::fmod(CurrentVisualNode->getRotation(), 180.0f));
	if ( Angle > 45.0f && Angle < 135.0f )
	{
		std::swap(CurrentSize.width, CurrentSize.height);
	}

	// Sprites are anchored in the middle, so the position is the center.
	return cocos2d::Rect(InPosition.x - CurrentSize.width / 2, InPosition.y - CurrentSize.height / 2, CurrentSize.width, CurrentSize.height);
}

bool PhysicsObject::IsCollidedTo(const GameObject* const InOther) const
{
	auto Other = ObjectCast<PhysicsObject>(InOther);
	if ( !Other || !GetVisualNode() || !Other->GetVisualNode() )
	{
		return false;
	}
	return GetCollisionRect().intersectsRect(Other->GetCollisionRect());
}

bool PhysicsObject::IsCollidedTo(const FGameObjectHandle& InOther) const
{
	return IsCollidedTo(InOther.Get());
}

bool PhysicsObject::Blocks(const cocos2d::Rect& InRect) const
{
	return InRect.intersectsRect(GetCollisionRect());
}

bool PhysicsObject::SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const
{
	const cocos2d::Rect Box = GetCollisionRect();
	const std::array<float, 2> Delta = { InTo.x - InFrom.x, InTo.y - InFrom.y };
	const std::array<float, 2> From = { InFrom.x, InFrom.y };
	const std::array<float, 2> Low = { Box.getMinX() - InHalf.x, Box.getMinY() - InHalf.y };
	const std::array<float, 2> High = { Box.getMaxX() + InHalf.x, Box.getMaxY() + InHalf.y };

	float Enter = 0.0f;
	float Exit = 1.0f;
	for ( int Axis = 0; Axis < 2; ++Axis )
	{
		if ( std::fabs(Delta[Axis]) < ParallelEpsilon )
		{
			if ( From[Axis] < Low[Axis] || From[Axis] > High[Axis] )
			{
				return false;
			}
			continue;
		}
		float T1 = (Low[Axis] - From[Axis]) / Delta[Axis];
		float T2 = (High[Axis] - From[Axis]) / Delta[Axis];
		if ( T1 > T2 )
		{
			std::swap(T1, T2);
		}
		Enter = std::max(Enter, T1);
		Exit = std::min(Exit, T2);
		if ( Enter > Exit )
		{
			return false;
		}
	}
	OutFraction = Enter;
	return true;
}

PhysicsObject* PhysicsObject::FindBlocker(const cocos2d::Rect& InRect) const
{
	FPhysicsHandle Found = GameContext::Get<ObjectsFactory>().FindObjectIf<PhysicsObject>([this, &InRect](PhysicsObject& InObject)
	{
		if (&InObject == this || InObject.IsDestroying() || InObject.IsPassable())
		{
			return false;
		}
		auto Other = ObjectCast<PhysicsObject>(&InObject);
		return Other && Other->GetVisualNode() && Other->Blocks(InRect);
	});
	return Found.Get();
}

PhysicsObject* PhysicsObject::FindBlocker(const cocos2d::Rect& InRect, const cocos2d::Rect& InCurrent) const
{
	FPhysicsHandle Found = GameContext::Get<ObjectsFactory>().FindObjectIf<PhysicsObject>([this, &InRect, &InCurrent](PhysicsObject& InObject)
	{
		if ( &InObject == this || InObject.IsDestroying() || InObject.IsPassable() )
		{
			return false;
		}
		auto Other = ObjectCast<PhysicsObject>(&InObject);
		if ( !Other || !Other->GetVisualNode() || !Other->Blocks(InRect) )
		{
			return false;
		}
		if ( !Other->Blocks(InCurrent) )
		{
			return true;
		}
		const cocos2d::Rect Box = Other->GetCollisionRect();
		return OverlapArea(InRect, Box) > OverlapArea(InCurrent, Box) + OverlapEpsilon;
	});
	return Found.Get();
}

bool PhysicsObject::IsInsideBounds(const cocos2d::Rect& InRect) const
{
	if ( !bHasBounds )
	{
		return true;
	}
	return InRect.getMinX() >= Bounds.getMinX() && InRect.getMaxX() <= Bounds.getMaxX()
		&& InRect.getMinY() >= Bounds.getMinY() && InRect.getMaxY() <= Bounds.getMaxY();
}

} // namespace game

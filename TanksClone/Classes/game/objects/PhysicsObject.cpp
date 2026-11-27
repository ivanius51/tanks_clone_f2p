#include "game/objects/PhysicsObject.h"

#include <algorithm>

#include "core/GameContext.h"
#include "game/ObjectsFactory.h"
#include "game/PhysicsManager.h"
#include "model/Collision.h"
#include "ui/CocosConvert.h"

namespace game
{

namespace
{
	constexpr float OverlapEpsilon = 0.01f;
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

	// Size of the visible sprite pixels.
	cocos2d::Size CurrentSize = CurrentVisualNode->getContentSize();
	if ( auto CurrentSprite = CheckedCast<cocos2d::Sprite>(CurrentVisualNode) )
	{
		if ( auto CurrentFrame = CurrentSprite->getSpriteFrame() )
		{
			CurrentSize = CurrentFrame->getRect().size;
		}
	}
	const model::Vec2f Center(InPosition.x, InPosition.y);
	return view::cocos::ToCocos(model::collision::MakeBodyRect(Center, model::Vec2f(CurrentSize.width, CurrentSize.height),
		model::Vec2f(CurrentVisualNode->getScaleX(), CurrentVisualNode->getScaleY()), CurrentVisualNode->getRotation(), GetType() == EObjectType::Vehicle));
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

PhysicsObject::~PhysicsObject()
{
	if ( PhysicsBody.IsValid() )
	{
		GameContext::Get<PhysicsManager>().GetWorld().DestroyBody(PhysicsBody);
	}
}

bool PhysicsObject::EnsureBody() const
{
	if ( !GetVisualNode() )
	{
		return false;
	}
	const model::RectF Box = view::cocos::FromCocos(GetCollisionRect());
	model::IPhysicsWorld& World = GameContext::Get<PhysicsManager>().GetWorld();
	if ( PhysicsBody.IsValid() && (BodySize.X != Box.Size.X || BodySize.Y != Box.Size.Y) )
	{
		World.DestroyBody(PhysicsBody);
		PhysicsBody = model::BodyId{};
	}
	if ( !PhysicsBody.IsValid() )
	{
		BodySize = Box.Size;
		model::BodyDef Definition;
		Definition.Type = model::BodyType::Kinematic;
		Definition.BodyShape.Kind = model::ShapeKind::Box;
		Definition.BodyShape.Anchor = model::Vec2f(0.0f, 0.0f);
		Definition.Profile.ObjectType = model::CollisionChannel::Pawn;
		Definition.BodyShape.Size = Box.Size;
		Definition.Position = Box.Origin;
		PhysicsBody = World.CreateBody(Definition);
		return true;
	}
	World.SetPosition(PhysicsBody, Box.Origin);
	return true;
}

bool PhysicsObject::SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const
{
	if ( !EnsureBody() )
	{
		return false;
	}
	return GameContext::Get<PhysicsManager>().GetWorld().SweepBody(PhysicsBody, model::Vec2f(InFrom.x, InFrom.y), model::Vec2f(InTo.x, InTo.y),
		model::Vec2f(InHalf.x, InHalf.y), OutFraction);
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
		return model::collision::OverlapArea(view::cocos::FromCocos(InRect), view::cocos::FromCocos(Box)) > model::collision::OverlapArea(view::cocos::FromCocos(InCurrent), view::cocos::FromCocos(Box)) + OverlapEpsilon;
	});
	return Found.Get();
}

bool PhysicsObject::IsInsideBounds(const cocos2d::Rect& InRect) const
{
	if ( !bHasBounds )
	{
		return true;
	}
	return model::collision::IsInside(view::cocos::FromCocos(Bounds), view::cocos::FromCocos(InRect));
}

} // namespace game

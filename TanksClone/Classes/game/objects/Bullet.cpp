#include "game/objects/Bullet.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "game/Effects.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"

namespace tanks
{

namespace
{
	constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;
	// Depth of the blast is counted in quarter tiles; the wall decides what one of its cells is.
	constexpr float ChipDepthUnit = TileSize / 4.0f;
	// The blast starts this far behind the contact point, so the face the bullet touched is inside it.
	constexpr float ContactSlack = 1.0f;

	model::HitSide SideFacing(const cocos2d::Vec2& InDirection)
	{
		if ( std::fabs(InDirection.y) > std::fabs(InDirection.x) )
		{
			return InDirection.y > 0.0f ? model::HitSide::Bottom : model::HitSide::Top;
		}
		return InDirection.x > 0.0f ? model::HitSide::Left : model::HitSide::Right;
	}

	cocos2d::Rect BlastArea(const cocos2d::Vec2& InContact, const cocos2d::Vec2& InDirection, const float InWidth, const float InDepth)
	{
		const cocos2d::Vec2 CurrentStart = InContact - InDirection * ContactSlack;
		const cocos2d::Vec2 End = InContact + InDirection * InDepth;
		const cocos2d::Vec2 Across(std::fabs(InDirection.y), std::fabs(InDirection.x));
		const cocos2d::Vec2 Low(std::min(CurrentStart.x, End.x), std::min(CurrentStart.y, End.y));
		const cocos2d::Vec2 High(std::max(CurrentStart.x, End.x), std::max(CurrentStart.y, End.y));
		const cocos2d::Vec2 Grow = Across * (InWidth / 2);
		return cocos2d::Rect(Low.x - Grow.x, Low.y - Grow.y, High.x - Low.x + 2 * Grow.x, High.y - Low.y + 2 * Grow.y);
	}
}

std::unique_ptr<Bullet> Bullet::Create(cocos2d::Node* const InNode)
{
	std::unique_ptr<Bullet> CurrentBullet(new Bullet());
	if ( !CurrentBullet->Build() )
	{
		return nullptr;
	}
	CurrentBullet->SetVisualNode(InNode);
	return CurrentBullet;
}

bool Bullet::Build()
{
	const bool bResult = PhysicsObject::Build();
	SetType(ObjectType::Bullet);
	SetPassable(true); // bullets never block vehicles
	StartUpdating();
	return bResult;
}

void Bullet::Configure(const cocos2d::ValueMap& InData)
{
	PhysicsObject::Configure(InData);
	MoveSpeed = DataConverter::GetFloat(InData, "speed", MoveSpeed);
	bExplodesOnHit = DataConverter::GetBool(InData, "explosion_on_hit", false);
	ChipWidth = DataConverter::GetFloat(InData, "chip_width", ChipWidth);
	ChipDepth = DataConverter::GetInt(InData, "chip_depth", ChipDepth);
	ChipDamage = DataConverter::GetInt(InData, "chip_damage", ChipDamage);
	bBreaksStone = DataConverter::GetBool(InData, "breaks_stone", bBreaksStone);
}

void Bullet::Update(const float InDelta)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return;
	}

	const float Radians = CurrentVisualNode->getRotation() * DegreesToRadians;
	const cocos2d::Vec2 CurrentDirection(std::sin(Radians), std::cos(Radians));
	const cocos2d::Vec2 CurrentStart = CurrentVisualNode->getPosition();
	const cocos2d::Vec2 End = CurrentStart + CurrentDirection * (InDelta * MoveSpeed);

	// The bullet is swept along its path as a box, so a fast bullet cannot jump over a thin wall
	// (or through a narrow part of a chipped one) between two frames.
	const cocos2d::Rect Self = GetCollisionRect();
	const cocos2d::Vec2 Half(Self.size.width / 2, Self.size.height / 2);

	PhysicsObject* CurrentTarget = nullptr;
	float BestT = 1.0f;
	ObjectsFactory::GetInstance().ForEachObject(ObjectType::Undefined, [&](GameObject& InObject)
	{
		if ( &InObject == this || InObject.IsDestroying() || InObject.GetType() == ObjectType::Bullet
			|| InObject.IsPassable() || InObject.GetTeam() == GetTeam() )
		{
			return;
		}
		auto Other = ObjectCast<PhysicsObject>(&InObject);
		if ( !Other || !Other->GetVisualNode() )
		{
			return;
		}

		float T = 0.0f;
		if ( Other->SweepHit(CurrentStart, End, Half, T) && T <= BestT )
		{
			BestT = T;
			CurrentTarget = Other;
		}
	});

	if ( CurrentTarget )
	{
		CurrentVisualNode->setPosition(CurrentStart + (End - CurrentStart) * BestT);
		Hit(CurrentTarget, CurrentDirection);
		return;
	}

	CurrentVisualNode->setPosition(End);
	if ( !IsInsideBounds(GetCollisionRect()) )
	{
		Destroy();
	}
}

bool Bullet::ChipWalls(const cocos2d::Vec2& InDirection)
{
	const cocos2d::Rect Self = GetCollisionRect();
	const float HalfAlong = std::fabs(InDirection.x) * Self.size.width / 2 + std::fabs(InDirection.y) * Self.size.height / 2;
	const cocos2d::Vec2 Contact = GetVisualNode()->getPosition() + InDirection * HalfAlong;
	const cocos2d::Rect Area = BlastArea(Contact, InDirection, ChipWidth, ChipDepth * ChipDepthUnit);
	const model::HitSide Side = SideFacing(InDirection);

	std::vector<Wall*> Walls;
	ObjectsFactory::GetInstance().ForEachObject(ObjectType::Wall, [&Walls](GameObject& InObject)
	{
		if ( !InObject.IsDestroying() )
		{
			Walls.push_back(static_cast<Wall*>(&InObject));
		}
	});

	int Removed = 0;
	for ( Wall* CurrentWall : Walls )
	{
		Removed += CurrentWall->Chip(Area, Side, ChipDepth, ChipDamage, bBreaksStone);
	}
	return Removed > 0;
}

void Bullet::Hit(PhysicsObject* const InTarget, const cocos2d::Vec2& InDirection)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( bExplodesOnHit )
	{
		effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), false);
	}

	if ( InTarget->GetType() == ObjectType::Wall )
	{
		const bool bChipped = ChipWalls(InDirection);
		AudioManager::GetInstance().Play(bChipped || InTarget->IsDestroyable() ? Sound::HitWall : Sound::HitArmor);
	}
	else
	{
		// Tanks play their own hit sound (it depends on their armor).
		if ( !ObjectCast<Tank>(InTarget) )
		{
			AudioManager::GetInstance().Play(InTarget->IsDestroyable() ? Sound::HitWall : Sound::HitArmor);
		}
		InTarget->ApplyDamage(GetDamage(), GetTeam());
	}
	Destroy();
}

} // namespace tanks

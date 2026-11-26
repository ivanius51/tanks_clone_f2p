#include "game/objects/Bullet.h"

#include <algorithm>
#include <cmath>

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/EventReport.h"
#include "model/BulletClash.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;
	// Depth of the blast is counted in quarter tiles; the wall decides what one of its cells is.
	constexpr float ChipDepthUnit = TileSize / 4.0f;
	// The blast starts this far behind the contact point, so the face the bullet touched is inside it.
	constexpr float ContactSlack = 1.0f;
	// A bounced bullet steps off the wall so its next sweep does not start inside it.
	constexpr float BounceBackOff = 2.0f;
	// A lobbed shell looks this much bigger at the top of its arc.
	constexpr float LobScaleGain = 0.8f;

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

FBulletHandle Bullet::Create(cocos2d::Node* const InNode)
{
	FBulletHandle CurrentBullet(new Bullet());
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
	SetType(EObjectType::Bullet);
	SetPassable(true); // bullets never block vehicles
	StartUpdating();
	return bResult;
}

cocos2d::Vec2 Bullet::GetVelocity() const
{
	const cocos2d::Node* CurrentVisualNode = GetVisualNode();
	if ( bLobbed || !CurrentVisualNode )
	{
		return cocos2d::Vec2::ZERO;
	}
	const float Radians = CurrentVisualNode->getRotation() * DegreesToRadians;
	return cocos2d::Vec2(std::sin(Radians), std::cos(Radians)) * MoveSpeed;
}

void Bullet::Configure(const cocos2d::ValueMap& InData)
{
	PhysicsObject::Configure(InData);
	MoveSpeed = DataConverter::GetFloat(InData, "speed", MoveSpeed) * DataConverter::GetFloat(InData, "speed_scale", 1.0f);
	bExplodesOnHit = DataConverter::GetBool(InData, "explosion_on_hit", false);
	bFriendlyFire = DataConverter::GetBool(InData, "friendly_fire", false);
	FriendlyFireShare = DataConverter::GetFloat(InData, "friendly_fire_share", FriendlyFireShare);
	bClash = DataConverter::GetBool(InData, "bullet_clash", bClash);
	ClashHealth = DataConverter::GetInt(InData, "bullet_health", ClashHealth);
	ChipWidth = DataConverter::GetFloat(InData, "chip_width", ChipWidth);
	ChipDepth = DataConverter::GetInt(InData, "chip_depth", ChipDepth);
	ChipDamage = DataConverter::GetInt(InData, "chip_damage", ChipDamage);
	bBreaksStone = DataConverter::GetBool(InData, "breaks_stone", bBreaksStone);
	Range = DataConverter::GetFloat(InData, "range_tiles", 0.0f) * TileSize;
	EdgeEffect = DataConverter::GetString(InData, "edge_effect", EdgeEffect);
	FadeEffect = DataConverter::GetString(InData, "fade_effect", FadeEffect);
	BouncesLeft = DataConverter::GetInt(InData, "bounces", 0);
	Lifetime = DataConverter::GetFloat(InData, "lifetime", 0.0f);
	bLobbed = DataConverter::GetBool(InData, "lobbed", false);
	BlastRadius = DataConverter::GetFloat(InData, "blast_radius_tiles", 0.0f) * TileSize;
	if ( bLobbed )
	{
		Lob.Start(DataConverter::GetFloat(InData, "lob_distance", Range > 0.0f ? Range : TileSize * 6.0f));
		BaseScale = DataConverter::GetFloat(InData, "scale", 1.0f);
	}
}

void Bullet::Update(const float InDelta)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return;
	}
	Age += InDelta;
	if ( Lifetime > 0.0f && Age >= Lifetime )
	{
		Destroy();
		return;
	}

	const float Radians = CurrentVisualNode->getRotation() * DegreesToRadians;
	const cocos2d::Vec2 CurrentDirection(std::sin(Radians), std::cos(Radians));
	if ( bLobbed )
	{
		UpdateLob(InDelta, CurrentDirection);
		return;
	}
	const cocos2d::Vec2 CurrentStart = CurrentVisualNode->getPosition();
	if ( !bFiredFromKnown )
	{
		FiredFrom = CurrentStart;
		bFiredFromKnown = true;
	}
	const cocos2d::Vec2 End = CurrentStart + CurrentDirection * (InDelta * MoveSpeed);

	// The bullet is swept along its path as a box, so a fast bullet cannot jump over a thin wall
	// (or through a narrow part of a chipped one) between two frames.
	const cocos2d::Rect Self = GetCollisionRect();
	const cocos2d::Vec2 Half(Self.size.width / 2, Self.size.height / 2);

	if ( ResolveClashes(CurrentStart, End, Half) )
	{
		return;
	}

	PhysicsObject* CurrentTarget = nullptr;
	float BestT = 1.0f;
	GameContext::Get<ObjectsFactory>().ForEachObject(EObjectType::Undefined, [&](const FGameObjectHandle& InObject)
	{
		if (InObject.Get() == this || InObject->IsDestroying() || InObject->GetType() == EObjectType::Bullet
			|| InObject->IsPassable() || (InObject->GetTeam() == GetTeam() && !bFriendlyFire) )
		{
			return;
		}
		auto Other = ObjectCast<PhysicsObject>(InObject.Get());
		if ( !Other || !Other->GetVisualNode() )
		{
			return;
		}
		if ( InObject->GetTeam() == GetTeam() && Other->GetCollisionRect().containsPoint(FiredFrom) )
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
	Travelled += InDelta * MoveSpeed;
	if ( !IsInsideBounds(GetCollisionRect()) )
	{
		effects::Play(EdgeEffect, CurrentVisualNode->getParent(), End, CurrentDirection);
		Destroy();
	}
	else if ( Range > 0.0f && Travelled >= Range )
	{
		effects::Play(FadeEffect, CurrentVisualNode->getParent(), End);
		Destroy();
	}
}

bool Bullet::ResolveClashes(const cocos2d::Vec2& InStart, const cocos2d::Vec2& InEnd, const cocos2d::Vec2& InHalf)
{
	TArray<Bullet*> Met;
	GameContext::Get<ObjectsFactory>().ForEachObject<Bullet>([&](Bullet& InOther)
	{
		if ( &InOther == this || InOther.IsDestroying() || InOther.bLobbed || InOther.GetTeam() == GetTeam()
			|| !(bClash || InOther.bClash) || !InOther.GetVisualNode() )
		{
			return;
		}
		float T = 0.0f;
		if ( InOther.SweepHit(InStart, InEnd, InHalf, T) )
		{
			Met.PushBack(&InOther);
		}
	});
	for ( Bullet* const Other : Met )
	{
		const model::ClashResult Result = model::Clash(ClashHealth, Other->ClashHealth);
		ClashHealth = Result.LeftA;
		Other->ClashHealth = Result.LeftB;
		if ( Other->ClashHealth <= 0 )
		{
			effects::Play(Other->FadeEffect, Other->GetVisualNode()->getParent(), Other->GetVisualNode()->getPosition());
			Other->Destroy();
		}
		if ( ClashHealth <= 0 )
		{
			effects::Play(FadeEffect, GetVisualNode()->getParent(), GetVisualNode()->getPosition());
			Destroy();
			return true;
		}
	}
	return false;
}

void Bullet::UpdateLob(const float InDelta, const cocos2d::Vec2& InDirection)
{
	auto CurrentVisualNode = GetVisualNode();
	const float Before = Lob.GetProgress();
	const bool bLanded = Lob.Advance(InDelta * MoveSpeed);
	const float Moved = (Lob.GetProgress() - Before) * Lob.GetDistance();
	CurrentVisualNode->setPosition(CurrentVisualNode->getPosition() + InDirection * Moved);
	CurrentVisualNode->setScale(BaseScale * (1.0f + LobScaleGain * Lob.GetHeight()));
	if ( bLanded || !IsInsideBounds(GetCollisionRect()) )
	{
		Land(InDirection);
	}
}

void Bullet::Land(const cocos2d::Vec2& InDirection)
{
	auto CurrentVisualNode = GetVisualNode();
	CurrentVisualNode->setScale(BaseScale);
	const cocos2d::Vec2 Center = CurrentVisualNode->getPosition();
	effects::Explosion(CurrentVisualNode->getParent(), Center, true);
	GameContext::Get<AudioManager>().Play(ids::sounds::HitWall);
	TArray<Tank*> Victims;
	GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
	{
		if ( !InTank.IsDestroying() && InTank.GetVisualNode() && InTank.GetTeam() != GetTeam()
			&& InTank.GetVisualNode()->getPosition().distance(Center) <= BlastRadius )
		{
			Victims.PushBack(&InTank);
		}
	});
	for ( Tank* Victim : Victims )
	{
		Victim->ApplyDamage(GetDamage(), GetTeam());
	}
	ChipWalls(InDirection);
	Destroy();
}

bool Bullet::ChipWalls(const cocos2d::Vec2& InDirection)
{
	const cocos2d::Rect Self = GetCollisionRect();
	const float HalfAlong = std::fabs(InDirection.x) * Self.size.width / 2 + std::fabs(InDirection.y) * Self.size.height / 2;
	const cocos2d::Vec2 Contact = GetVisualNode()->getPosition() + InDirection * HalfAlong;
	const cocos2d::Rect Area = BlastArea(Contact, InDirection, ChipWidth, ChipDepth * ChipDepthUnit);
	const model::HitSide Side = SideFacing(InDirection);

	TArray<Wall*> Walls;
	GameContext::Get<ObjectsFactory>().ForEachObject<Wall>([&Walls](Wall& InObject)
	{
		if ( !InObject.IsDestroying() )
		{
			Walls.PushBack(&InObject);
		}
	});

	int Removed = 0;
	for ( Wall* CurrentWall : Walls )
	{
		Removed += CurrentWall->Chip(Area, Side, ChipDepth, ChipDamage, bBreaksStone);
	}
	return Removed > 0;
}

cocos2d::Vec2 Bullet::GetImpactPoint(const PhysicsObject& InTarget, const cocos2d::Vec2& InDirection) const
{
	cocos2d::Vec2 Point = GetVisualNode()->getPosition();
	const cocos2d::Rect Box = InTarget.GetCollisionRect();
	if ( std::abs(InDirection.x) > std::abs(InDirection.y) )
	{
		Point.x = InDirection.x > 0.0f ? Box.getMinX() : Box.getMaxX();
	}
	else
	{
		Point.y = InDirection.y > 0.0f ? Box.getMinY() : Box.getMaxY();
	}
	return Point;
}

void Bullet::Hit(PhysicsObject* const InTarget, const cocos2d::Vec2& InDirection)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( bExplodesOnHit )
	{
		effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), false);
	}
	else
	{
		effects::Play(InTarget->GetHitEffect(), CurrentVisualNode->getParent(), GetImpactPoint(*InTarget, InDirection), InDirection);
	}

	if ( InTarget->GetType() == EObjectType::Wall && BouncesLeft > 0 )
	{
		--BouncesLeft;
		events::Report("bullet.bounced");
		CurrentVisualNode->setRotation(CurrentVisualNode->getRotation() + 180.0f);
		CurrentVisualNode->setPosition(CurrentVisualNode->getPosition() - InDirection * BounceBackOff);
		GameContext::Get<AudioManager>().Play(ids::sounds::HitArmor);
		return;
	}
	if ( InTarget->GetType() == EObjectType::Wall )
	{
		events::Report("bullet.hit_wall");
		const bool bChipped = ChipWalls(InDirection);
		GameContext::Get<AudioManager>().Play(bChipped || InTarget->IsDestroyable() ? ids::sounds::HitWall : ids::sounds::HitArmor);
	}
	else
	{
		// Tanks play their own hit sound (it depends on their armor).
		if ( !ObjectCast<Tank>(InTarget) )
		{
			GameContext::Get<AudioManager>().Play(InTarget->IsDestroyable() ? ids::sounds::HitWall : ids::sounds::HitArmor);
		}
		if ( auto TargetTank = ObjectCast<Tank>(InTarget) )
		{
			events::Report("bullet.hit_tank");
			const int ShotDamage = TargetTank->GetTeam() == GetTeam() ? model::FriendlyDamage(GetDamage(), FriendlyFireShare) : GetDamage();
			TargetTank->ApplyDamageFrom(ShotDamage, GetTeam(), InDirection);
		}
		else
		{
			InTarget->ApplyDamage(GetDamage(), GetTeam());
		}
	}
	Destroy();
}

} // namespace game

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
#include "game/PhysicsManager.h"
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

	constexpr float DefaultHitInsetTiles = 1.0f / 16.0f;
	constexpr int DefaultHitInsetSides = 1 << static_cast<int>(model::HitSide::Bottom);

int ParseShotSides(const model::JsonValue& InList)
	{
		int Mask = 0;
		for ( const model::JsonValue& Item : InList.GetArray() )
		{
			const std::string Name = Item.AsString();
			if ( Name == "up" )
			{
				Mask |= 1 << static_cast<int>(model::HitSide::Bottom);
			}
			else if ( Name == "down" )
			{
				Mask |= 1 << static_cast<int>(model::HitSide::Top);
			}
			else if ( Name == "right" )
			{
				Mask |= 1 << static_cast<int>(model::HitSide::Left);
			}
			else if ( Name == "left" )
			{
				Mask |= 1 << static_cast<int>(model::HitSide::Right);
			}
		}
		return Mask;
	}

model::HitSide SideFacing(const cocos2d::Vec2& InDirection)
	{
		if ( std::fabs(InDirection.y) > std::fabs(InDirection.x) )
		{
			return InDirection.y > 0.0f ? model::HitSide::Bottom : model::HitSide::Top;
		}
		return InDirection.x > 0.0f ? model::HitSide::Left : model::HitSide::Right;
	}

	constexpr float FlightEpsilon = 0.01f;

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

Bullet::~Bullet()
{
	if ( FlightBody.IsValid() )
	{
		GameContext::Get<PhysicsManager>().Forget(FlightBody);
		GameContext::Get<PhysicsManager>().GetWorld().DestroyBody(FlightBody);
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

void Bullet::Configure(const model::JsonValue::Object& InData)
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
	if ( const model::JsonValue* const OriginValue = InData.Find("origin") )
	{
		Origin = DataConverter::ValueToVec2(*OriginValue);
		bHasOrigin = true;
		FiredFrom = Origin;
		bFiredFromKnown = true;
	}
	HitInset = DataConverter::GetFloat(InData, "hit_inset_tiles", DefaultHitInsetTiles) * TileSize;
	HitInsetSides = DefaultHitInsetSides;
	if ( const model::JsonValue* const SidesValue = InData.Find("hit_inset_sides") )
	{
		HitInsetSides = ParseShotSides(*SidesValue);
	}
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

	GameContext::Get<PhysicsManager>().SyncBodies();

	// The first move starts at the shooter, so a wall touching the tank is met at its surface instead of being skipped from inside.
	const cocos2d::Vec2 FlightStart = bHasOrigin && !bSweptFromOrigin ? Origin : CurrentStart;
	bSweptFromOrigin = true;
	PhysicsManager& Physics = GameContext::Get<PhysicsManager>();
	model::IPhysicsWorld& World = Physics.GetWorld();
	const model::BodyId Flight = EnsureFlightBody(Half);
	World.SetPosition(Flight, model::Vec2f(FlightStart.x, FlightStart.y));
	World.SetVelocity(Flight, model::Vec2f(End.x - FlightStart.x, End.y - FlightStart.y) / InDelta);
	FlightDirection = CurrentDirection;
	World.StepBody(Flight, InDelta);
	const model::Vec2f Reached = World.GetPosition(Flight);
	CurrentVisualNode->setPosition(cocos2d::Vec2(Reached.X, Reached.Y));
	Physics.Pump();
	if ( IsDestroying() || CurrentVisualNode->getPosition().distance(End) > FlightEpsilon )
	{
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

model::BodyId Bullet::EnsureFlightBody(const cocos2d::Vec2& InHalf, const bool bInPassesThrough)
{
	if ( FlightBody.IsValid() )
	{
		return FlightBody;
	}
	model::BodyDef Definition;
	Definition.Type = model::BodyType::Dynamic;
	Definition.BodyShape.Kind = model::ShapeKind::Box;
	Definition.BodyShape.Size = model::Vec2f(InHalf.x * 2.0f, InHalf.y * 2.0f);
	Definition.BodyShape.Anchor = model::Vec2f(0.5f, 0.5f);
	Definition.Profile.ObjectType = model::CollisionChannel::Projectile;
	Definition.Profile.SetResponseTo(model::CollisionChannel::WorldDynamic, model::CollisionResponse::Ignore);
	Definition.Profile.SetResponseTo(model::CollisionChannel::Projectile, model::CollisionResponse::Ignore);
	if ( bInPassesThrough )
	{
		Definition.Profile.SetResponseTo(model::CollisionChannel::WorldStatic, model::CollisionResponse::Ignore);
		Definition.Profile.SetResponseTo(model::CollisionChannel::Pawn, model::CollisionResponse::Ignore);
	}
	Definition.Group = static_cast<unsigned>(GetTeam()) + 1;
	Definition.bHitsOwnGroup = bFriendlyFire;
	Definition.bHasIgnorePoint = true;
	Definition.IgnorePoint = model::Vec2f(FiredFrom.x, FiredFrom.y);
	PhysicsManager& Physics = GameContext::Get<PhysicsManager>();
	FlightBody = Physics.GetWorld().CreateBody(Definition);
	Physics.Listen(FlightBody, this);
	return FlightBody;
}

void Bullet::OnContact(const model::ContactEvent& InEvent)
{
	PhysicsObject* const Target = GameContext::Get<PhysicsManager>().FindObject(InEvent.Hit.Item);
	if ( Target && !IsDestroying() )
	{
		ImpactPoint = cocos2d::Vec2(InEvent.Hit.ImpactPoint.X, InEvent.Hit.ImpactPoint.Y);
		Hit(Target, FlightDirection);
	}
}

bool Bullet::ResolveClashes(const cocos2d::Vec2& InStart, const cocos2d::Vec2& InEnd, const cocos2d::Vec2& InHalf)
{
	PhysicsManager& Physics = GameContext::Get<PhysicsManager>();
	TArray<model::BodyId> Bodies;
	Physics.GetWorld().SweepAll(model::Vec2f(InStart.x, InStart.y), model::Vec2f(InEnd.x, InEnd.y), model::Vec2f(InHalf.x, InHalf.y), [&](const model::BodyId InBody)
	{
		PhysicsObject* const Found = Physics.FindObject(InBody);
		if ( !Found || Found == this || Found->GetType() != EObjectType::Bullet )
		{
			return false;
		}
		const Bullet* const OtherBullet = ObjectCast<Bullet>(Found);
		return !OtherBullet->IsDestroying() && !OtherBullet->bLobbed && OtherBullet->GetTeam() != GetTeam() && (bClash || OtherBullet->bClash) && OtherBullet->GetVisualNode();
	}, Bodies);
	TArray<Bullet*> Met;
	for ( const model::BodyId Body : Bodies )
	{
		Met.PushBack(ObjectCast<Bullet>(Physics.FindObject(Body)));
	}
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
	// A lobbed shell flies over everything, so its body ignores all channels and only carries the position.
	model::IPhysicsWorld& World = GameContext::Get<PhysicsManager>().GetWorld();
	const cocos2d::Vec2 Start = CurrentVisualNode->getPosition();
	const model::BodyId Flight = EnsureFlightBody(GetCollisionRect().size * 0.5f, true);
	World.SetPosition(Flight, model::Vec2f(Start.x, Start.y));
	World.SetVelocity(Flight, model::Vec2f(InDirection.x, InDirection.y) * (Moved / InDelta));
	World.StepBody(Flight, InDelta);
	const model::Vec2f Reached = World.GetPosition(Flight);
	CurrentVisualNode->setPosition(cocos2d::Vec2(Reached.X, Reached.Y));
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
	PhysicsManager& Physics = GameContext::Get<PhysicsManager>();
	GameContext::Get<PhysicsManager>().SyncBodies();
	TArray<model::BodyId> Bodies;
	Physics.GetWorld().QueryRadius(model::Vec2f(Center.x, Center.y), BlastRadius, [&](const model::BodyId InBody)
	{
		PhysicsObject* const Found = Physics.FindObject(InBody);
		return Found && ObjectCast<Tank>(Found) && !Found->IsDestroying() && Found->GetTeam() != GetTeam();
	}, Bodies);
	TArray<Tank*> Victims;
	for ( const model::BodyId Body : Bodies )
	{
		Victims.PushBack(ObjectCast<Tank>(Physics.FindObject(Body)));
	}
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

cocos2d::Vec2 Bullet::GetImpactPoint(const cocos2d::Vec2& InDirection) const
{
	const bool bInset = (HitInsetSides & (1 << static_cast<int>(SideFacing(InDirection)))) != 0;
	return bInset ? ImpactPoint + InDirection * HitInset : ImpactPoint;
}

void Bullet::Hit(PhysicsObject* const InTarget, const cocos2d::Vec2& InDirection)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( bExplodesOnHit )
	{
		effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), false);
	}

	const bool bWall = InTarget->GetType() == EObjectType::Wall;
	if ( !bExplodesOnHit && bWall && BouncesLeft > 0 )
	{
		effects::Play(InTarget->GetHitEffect().View(), CurrentVisualNode->getParent(), GetImpactPoint(InDirection), InDirection);
	}

	const model::ProjectileHitOutcome Outcome = model::ResolveProjectileHit({ bWall ? model::ProjectileTarget::Wall : model::ProjectileTarget::Other, BouncesLeft, GetDamage(), InTarget->GetTeam() == GetTeam(), FriendlyFireShare });
	if ( Outcome.Action == model::ProjectileAction::Bounce )
	{
		BouncesLeft = Outcome.BouncesLeft;
		events::Report("bullet.bounced");
		CurrentVisualNode->setRotation(CurrentVisualNode->getRotation() + 180.0f);
		CurrentVisualNode->setPosition(CurrentVisualNode->getPosition() - InDirection * BounceBackOff);
		GameContext::Get<AudioManager>().Play(ids::sounds::HitArmor);
		return;
	}
	if ( Outcome.Action == model::ProjectileAction::ChipWall )
	{
		events::Report("bullet.hit_wall");
		const bool bChipped = ChipWalls(InDirection);
		if ( !bExplodesOnHit && !bChipped )
		{
			effects::Play(InTarget->GetHitEffect().View(), CurrentVisualNode->getParent(), GetImpactPoint(InDirection), InDirection);
		}
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
			TargetTank->ApplyDamageFrom(Outcome.Damage, GetTeam(), InDirection);
		}
		else
		{
			InTarget->ApplyDamage(GetDamage(), GetTeam());
		}
		if ( !bExplodesOnHit && !InTarget->IsDestroying() )
		{
			effects::Play(InTarget->GetHitEffect().View(), CurrentVisualNode->getParent(), GetImpactPoint(InDirection), InDirection);
		}
	}
	Destroy();
}

} // namespace game

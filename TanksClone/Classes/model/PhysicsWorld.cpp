#include "model/PhysicsWorld.h"

#include <algorithm>
#include <cmath>

#include "model/Collision.h"

namespace game::model
{
namespace
{
constexpr float MinMoveSquared = 1e-10f;

CollisionResponse Resolve(const CollisionProfile& InFirst, const CollisionProfile& InSecond)
{
	return std::min(InFirst.GetResponseTo(InSecond.ObjectType), InSecond.GetResponseTo(InFirst.ObjectType));
}

Vec2f SurfaceNormal(const RectF& InBox, const Vec2f& InHalf, const Vec2f& InLocation)
{
	const float Left = std::fabs(InLocation.X - (InBox.GetMinX() - InHalf.X));
	const float Right = std::fabs(InLocation.X - (InBox.GetMaxX() + InHalf.X));
	const float Bottom = std::fabs(InLocation.Y - (InBox.GetMinY() - InHalf.Y));
	const float Top = std::fabs(InLocation.Y - (InBox.GetMaxY() + InHalf.Y));
	const float Nearest = std::min({ Left, Right, Bottom, Top });
	if ( Nearest == Left )
	{
		return Vec2f(-1.0f, 0.0f);
	}
	if ( Nearest == Right )
	{
		return Vec2f(1.0f, 0.0f);
	}
	return Nearest == Bottom ? Vec2f(0.0f, -1.0f) : Vec2f(0.0f, 1.0f);
}
}

BodyId PhysicsWorld::CreateBody(const BodyDef& InDefinition)
{
	Body NewBody;
	NewBody.Definition = InDefinition;
	if ( InDefinition.BodyShape.Kind == ShapeKind::Grid )
	{
		NewBody.Grid.emplace(std::max(1, InDefinition.BodyShape.GridCellsPerSide), std::max(1, InDefinition.BodyShape.GridCellHealth));
	}
	Bodies.PushBack(std::move(NewBody));
	return BodyId{ static_cast<unsigned>(Bodies.Size()) };
}

void PhysicsWorld::DestroyBody(const BodyId InBody)
{
	if ( Body* const Found = Find(InBody) )
	{
		Found->bAlive = false;
	}
}

Vec2f PhysicsWorld::GetPosition(const BodyId InBody) const
{
	const Body* const Found = Find(InBody);
	return Found ? Found->Definition.Position : Vec2f();
}

void PhysicsWorld::SetPosition(const BodyId InBody, const Vec2f& InPosition)
{
	if ( Body* const Found = Find(InBody) )
	{
		Found->Definition.Position = InPosition;
	}
}

void PhysicsWorld::SetVelocity(const BodyId InBody, const Vec2f& InVelocity)
{
	if ( Body* const Found = Find(InBody) )
	{
		Found->Velocity = InVelocity;
	}
}

PhysicsWorld::Body* PhysicsWorld::Find(const BodyId InBody)
{
	return InBody.IsValid() && InBody.Value <= Bodies.Size() && Bodies[InBody.Value - 1].bAlive ? &Bodies[InBody.Value - 1] : nullptr;
}

const PhysicsWorld::Body* PhysicsWorld::Find(const BodyId InBody) const
{
	return InBody.IsValid() && InBody.Value <= Bodies.Size() && Bodies[InBody.Value - 1].bAlive ? &Bodies[InBody.Value - 1] : nullptr;
}

RectF PhysicsWorld::GetBox(const Body& InBody)
{
	const Shape& BodyShape = InBody.Definition.BodyShape;
	const Vec2f Origin(InBody.Definition.Position.X - BodyShape.Size.X * BodyShape.Anchor.X, InBody.Definition.Position.Y - BodyShape.Size.Y * BodyShape.Anchor.Y);
	return RectF(Origin, BodyShape.Size);
}

bool PhysicsWorld::SweepSingle(const Vec2f& InStart, const Vec2f& InEnd, const Shape& InShape, const CollisionProfile& InProfile, HitResult& OutHit) const
{
	return Sweep(InStart, InEnd, InShape, InProfile, BodyId{}, OutHit);
}

bool PhysicsWorld::Sweep(const Vec2f& InStart, const Vec2f& InEnd, const Shape& InShape, const CollisionProfile& InProfile, const BodyId InIgnored, HitResult& OutHit) const
{
	const Vec2f Half = InShape.Size / 2.0f;
	OutHit = HitResult();
	OutHit.Location = InEnd;
	bool bHit = false;
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		const Body& Other = Bodies[Index];
		const BodyId OtherId{ static_cast<unsigned>(Index + 1) };
		if ( !Other.bAlive || OtherId == InIgnored || Resolve(InProfile, Other.Definition.Profile) != CollisionResponse::Block )
		{
			continue;
		}
		const RectF Box = GetBox(Other);
		float Fraction = 0.0f;
		const bool bFound = Other.Grid ? Other.Grid->SweepHit(Box, InStart, InEnd, Half, Fraction) : collision::SweepBox(Box, InStart, InEnd, Half, Fraction);
		if ( !bFound || (bHit && Fraction >= OutHit.Time) )
		{
			continue;
		}
		bHit = true;
		OutHit.bBlockingHit = true;
		OutHit.bStartPenetrating = Fraction <= 0.0f;
		OutHit.Time = Fraction;
		OutHit.Item = OtherId;
		OutHit.Location = InStart + (InEnd - InStart) * Fraction;
		OutHit.Normal = SurfaceNormal(Box, Half, OutHit.Location);
		OutHit.ImpactPoint = OutHit.Location - OutHit.Normal * (std::fabs(OutHit.Normal.X) * Half.X + std::fabs(OutHit.Normal.Y) * Half.Y);
	}
	OutHit.Distance = InStart.DistanceTo(OutHit.Location);
	return bHit;
}

const CellGrid* PhysicsWorld::GetGrid(const BodyId InBody) const
{
	const Body* const Found = Find(InBody);
	return Found && Found->Grid ? &Found->Grid->GetMask() : nullptr;
}

bool PhysicsWorld::TestOverlap(const BodyId InBody, const RectF& InArea) const
{
	const Body* const Found = Find(InBody);
	if ( !Found )
	{
		return false;
	}
	return Found->Grid ? Found->Grid->Blocks(GetBox(*Found), InArea) : GetBox(*Found).Intersects(InArea);
}

bool PhysicsWorld::SweepBody(const BodyId InBody, const Vec2f& InStart, const Vec2f& InEnd, const Vec2f& InHalf, float& OutTime) const
{
	const Body* const Found = Find(InBody);
	if ( !Found )
	{
		return false;
	}
	return Found->Grid ? Found->Grid->SweepHit(GetBox(*Found), InStart, InEnd, InHalf, OutTime) : collision::SweepBox(GetBox(*Found), InStart, InEnd, InHalf, OutTime);
}

GridCarveResult PhysicsWorld::Carve(const BodyId InBody, const RectF& InArea, const HitSide InSide, const int InDepthQuarters, const int InDamage)
{
	GridCarveResult Result;
	Body* const Found = Find(InBody);
	if ( !Found || !Found->Grid )
	{
		return Result;
	}
	Found->Grid->Chip(GetBox(*Found), InArea, InSide, InDepthQuarters, InDamage, Result);
	if ( Result.Removed <= 0 )
	{
		return Result;
	}
	CarveEvent Event;
	Event.Body = InBody;
	Event.BodyTag = Found->Definition.UserTag;
	Event.Point = InArea.GetCenter();
	Event.Result = Result;
	Event.bEmptied = Found->Grid->GetMask().IsEmpty();
	Events.OnCarved(Event);
	return Result;
}

void PhysicsWorld::Step(const float InDeltaSeconds)
{
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		Body& Mover = Bodies[Index];
		if ( !Mover.bAlive || Mover.Definition.Type == BodyType::Static || Mover.Velocity.LengthSquared() < MinMoveSquared )
		{
			continue;
		}
		const BodyId Id{ static_cast<unsigned>(Index + 1) };
		const Vec2f Delta = Mover.Velocity * InDeltaSeconds;
		if ( Mover.Definition.Type == BodyType::Kinematic )
		{
			Mover.Definition.Position += Delta;
			continue;
		}
		const Shape& MoverShape = Mover.Definition.BodyShape;
		const Vec2f CenterOffset(MoverShape.Size.X * (0.5f - MoverShape.Anchor.X), MoverShape.Size.Y * (0.5f - MoverShape.Anchor.Y));
		const Vec2f Start = Mover.Definition.Position + CenterOffset;
		HitResult Hit;
		if ( !Sweep(Start, Start + Delta, MoverShape, Mover.Definition.Profile, Id, Hit) )
		{
			Mover.Definition.Position += Delta;
			continue;
		}
		Mover.Definition.Position = Hit.Location - CenterOffset;
		Mover.Velocity = Vec2f();
		ContactEvent Event;
		Event.Kind = ContactKind::Hit;
		Event.Self = Id;
		Event.SelfTag = Mover.Definition.UserTag;
		Event.OtherTag = Find(Hit.Item) ? Find(Hit.Item)->Definition.UserTag : 0;
		Event.Hit = Hit;
		Events.OnContact(Event);
	}
}

void PhysicsWorld::DrainEvents(IPhysicsEventSink& InSink)
{
	Events.Drain(InSink);
}
} // namespace game::model

#include "model/PhysicsWorld.h"

#include <algorithm>
#include <cmath>

#include "model/Collision.h"

namespace game::model
{
namespace
{
constexpr float MinMoveSquared = 1e-10f;
// A body that slides starts its next move this far off the surface, since a touching box counts as a hit.
constexpr float SlideSkin = 0.01f;

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
	if ( !FreeSlots.IsEmpty() )
	{
		const unsigned Slot = FreeSlots.Back();
		FreeSlots.PopBack();
		Bodies[Slot] = std::move(NewBody);
		return BodyId{ Slot + 1 };
	}
	Bodies.PushBack(std::move(NewBody));
	return BodyId{ static_cast<unsigned>(Bodies.Size()) };
}

void PhysicsWorld::DestroyBody(const BodyId InBody)
{
	if ( Body* const Found = Find(InBody) )
	{
		Found->bAlive = false;
		Found->Grid.reset();
		Found->Overlapping.Clear();
		FreeSlots.PushBack(InBody.Value - 1);
	}
}

void PhysicsWorld::SetBodyFilter(const BodyId InBody, const CollisionChannel InObjectType, const unsigned InGroup)
{
	if ( Body* const Found = Find(InBody) )
	{
		Found->Definition.Profile.ObjectType = InObjectType;
		Found->Definition.Group = InGroup;
	}
}

void PhysicsWorld::SetEnabled(const BodyId InBody, const bool bInEnabled)
{
	if ( Body* const Found = Find(InBody) )
	{
		Found->bEnabled = bInEnabled;
	}
}

void PhysicsWorld::StepBody(const BodyId InBody, const float InDeltaSeconds)
{
	if ( Find(InBody) )
	{
		Advance(InBody.Value - 1, InDeltaSeconds);
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
	BodyDef Mover;
	Mover.Profile = InProfile;
	return Sweep(InStart, InEnd, InShape, Mover, BodyId{}, OutHit);
}

bool PhysicsWorld::IsFilteredOut(const BodyDef& InMover, const Body& InOther)
{
	if ( InMover.Group == 0 || InOther.Definition.Group != InMover.Group )
	{
		return false;
	}
	return !InMover.bHitsOwnGroup || (InMover.bHasIgnorePoint && GetBox(InOther).Contains(InMover.IgnorePoint));
}

bool PhysicsWorld::Sweep(const Vec2f& InStart, const Vec2f& InEnd, const Shape& InShape, const BodyDef& InMover, const BodyId InIgnored, HitResult& OutHit) const
{
	const Vec2f Half = InShape.Size / 2.0f;
	OutHit = HitResult();
	OutHit.Location = InEnd;
	bool bHit = false;
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		const Body& Other = Bodies[Index];
		const BodyId OtherId{ static_cast<unsigned>(Index + 1) };
		if ( !Other.bAlive || !Other.bEnabled || OtherId == InIgnored || IsFilteredOut(InMover, Other) || Resolve(InMover.Profile, Other.Definition.Profile) != CollisionResponse::Block )
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

void PhysicsWorld::ReportContact(const ContactEvent& InEvent)
{
	Events.OnContact(InEvent);
}

bool PhysicsWorld::SweepFiltered(const Vec2f& InStart, const Vec2f& InEnd, const Vec2f& InHalf, TFunctionRef<bool(BodyId)> InAccept, HitResult& OutHit) const
{
	OutHit = HitResult();
	OutHit.Location = InEnd;
	bool bHit = false;
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		const Body& Other = Bodies[Index];
		const BodyId OtherId{ static_cast<unsigned>(Index + 1) };
		if ( !Other.bAlive || !InAccept(OtherId) )
		{
			continue;
		}
		const RectF Box = GetBox(Other);
		float Fraction = 0.0f;
		const bool bFound = Other.Grid ? Other.Grid->SweepHit(Box, InStart, InEnd, InHalf, Fraction) : collision::SweepBox(Box, InStart, InEnd, InHalf, Fraction);
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
		OutHit.Normal = SurfaceNormal(Box, InHalf, OutHit.Location);
		OutHit.ImpactPoint = OutHit.Location - OutHit.Normal * (std::fabs(OutHit.Normal.X) * InHalf.X + std::fabs(OutHit.Normal.Y) * InHalf.Y);
	}
	OutHit.Distance = InStart.DistanceTo(OutHit.Location);
	return bHit;
}

void PhysicsWorld::SweepAll(const Vec2f& InStart, const Vec2f& InEnd, const Vec2f& InHalf, TFunctionRef<bool(BodyId)> InAccept, TArray<BodyId>& OutBodies) const
{
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		const Body& Other = Bodies[Index];
		const BodyId OtherId{ static_cast<unsigned>(Index + 1) };
		if ( !Other.bAlive || !Other.bEnabled || !InAccept(OtherId) )
		{
			continue;
		}
		float Fraction = 0.0f;
		if ( collision::SweepBox(GetBox(Other), InStart, InEnd, InHalf, Fraction) )
		{
			OutBodies.PushBack(OtherId);
		}
	}
}

void PhysicsWorld::QueryRadius(const Vec2f& InCenter, const float InRadius, TFunctionRef<bool(BodyId)> InAccept, TArray<BodyId>& OutBodies) const
{
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		const Body& Other = Bodies[Index];
		const BodyId OtherId{ static_cast<unsigned>(Index + 1) };
		if ( !Other.bAlive || !Other.bEnabled || !InAccept(OtherId) )
		{
			continue;
		}
		if ( GetBox(Other).GetCenter().DistanceTo(InCenter) <= InRadius )
		{
			OutBodies.PushBack(OtherId);
		}
	}
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
		Advance(Index, InDeltaSeconds);
	}
	for ( size_t Index = 0; Index < Bodies.Size(); ++Index )
	{
		UpdateOverlaps(Index);
	}
}

void PhysicsWorld::Advance(const size_t InIndex, const float InDeltaSeconds)
{
	Body& Mover = Bodies[InIndex];
	if ( !Mover.bAlive || !Mover.bEnabled || Mover.Definition.Type == BodyType::Static || Mover.Velocity.LengthSquared() < MinMoveSquared )
	{
		return;
	}
	const BodyId Id{ static_cast<unsigned>(InIndex + 1) };
	const Vec2f Delta = Mover.Velocity * InDeltaSeconds;
	if ( Mover.Definition.Type == BodyType::Kinematic )
	{
		Mover.Definition.Position += Delta;
		return;
	}
	const Shape& MoverShape = Mover.Definition.BodyShape;
	const Vec2f CenterOffset(MoverShape.Size.X * (0.5f - MoverShape.Anchor.X), MoverShape.Size.Y * (0.5f - MoverShape.Anchor.Y));
	const Vec2f Start = Mover.Definition.Position + CenterOffset;
	HitResult Hit;
	if ( !Sweep(Start, Start + Delta, MoverShape, Mover.Definition, Id, Hit) )
	{
		Mover.Definition.Position += Delta;
		return;
	}
	Mover.Definition.Position = Hit.Location - CenterOffset;
	QueueHit(Mover, Id, Hit);
	if ( !Mover.Definition.bSlides )
	{
		Mover.Velocity = Vec2f();
		return;
	}
	Slide(Mover, Id, Hit, Delta * (1.0f - Hit.Time), CenterOffset);
}

void PhysicsWorld::QueueHit(const Body& InMover, const BodyId InId, const HitResult& InHit)
{
	ContactEvent Event;
	Event.Kind = ContactKind::Hit;
	Event.Self = InId;
	Event.SelfTag = InMover.Definition.UserTag;
	Event.OtherTag = Find(InHit.Item) ? Find(InHit.Item)->Definition.UserTag : 0;
	Event.Hit = InHit;
	Events.OnContact(Event);
}

void PhysicsWorld::Slide(Body& InMover, const BodyId InId, const HitResult& InHit, const Vec2f& InRemaining, const Vec2f& InCenterOffset)
{
	const float IntoSurface = InMover.Velocity.Dot(InHit.Normal);
	if ( IntoSurface < 0.0f )
	{
		InMover.Velocity -= InHit.Normal * IntoSurface;
	}
	const Vec2f Along = InRemaining - InHit.Normal * InRemaining.Dot(InHit.Normal);
	if ( Along.LengthSquared() < MinMoveSquared )
	{
		return;
	}
	const Vec2f From = InHit.Location + InHit.Normal * SlideSkin;
	HitResult Second;
	if ( !Sweep(From, From + Along, InMover.Definition.BodyShape, InMover.Definition, InId, Second) )
	{
		InMover.Definition.Position = From + Along - InCenterOffset;
		return;
	}
	InMover.Definition.Position = Second.Location - InCenterOffset;
	QueueHit(InMover, InId, Second);
	const float IntoSecond = InMover.Velocity.Dot(Second.Normal);
	if ( IntoSecond < 0.0f )
	{
		InMover.Velocity -= Second.Normal * IntoSecond;
	}
}

bool PhysicsWorld::IsOverlapping(const Body& InFirst, const Body& InSecond) const
{
	if ( Resolve(InFirst.Definition.Profile, InSecond.Definition.Profile) != CollisionResponse::Overlap )
	{
		return false;
	}
	const RectF Box = GetBox(InFirst);
	return InSecond.Grid ? InSecond.Grid->Blocks(GetBox(InSecond), Box) : GetBox(InSecond).Intersects(Box);
}

void PhysicsWorld::UpdateOverlaps(const size_t InIndex)
{
	Body& Self = Bodies[InIndex];
	if ( !Self.bAlive || !Self.bEnabled || !Self.Definition.bGeneratesOverlapEvents )
	{
		return;
	}
	const BodyId Id{ static_cast<unsigned>(InIndex + 1) };
	TArray<BodyId> Now;
	for ( size_t Other = 0; Other < Bodies.Size(); ++Other )
	{
		const Body& Candidate = Bodies[Other];
		if ( Other != InIndex && Candidate.bAlive && Candidate.bEnabled && IsOverlapping(Self, Candidate) )
		{
			Now.PushBack(BodyId{ static_cast<unsigned>(Other + 1) });
		}
	}
	const auto Contains = [](const TArray<BodyId>& InList, const BodyId InItem)
	{
		for ( const BodyId Entry : InList )
		{
			if ( Entry == InItem )
			{
				return true;
			}
		}
		return false;
	};
	const auto Report = [&](const ContactKind InKind, const BodyId InOther)
	{
		ContactEvent Event;
		Event.Kind = InKind;
		Event.Self = Id;
		Event.SelfTag = Self.Definition.UserTag;
		Event.OtherTag = Bodies[InOther.Value - 1].Definition.UserTag;
		Event.Hit.Item = InOther;
		Events.OnContact(Event);
	};
	for ( const BodyId Entry : Now )
	{
		if ( !Contains(Self.Overlapping, Entry) )
		{
			Report(ContactKind::OverlapBegin, Entry);
		}
	}
	for ( const BodyId Entry : Self.Overlapping )
	{
		if ( !Contains(Now, Entry) )
		{
			Report(ContactKind::OverlapEnd, Entry);
		}
	}
	Self.Overlapping = std::move(Now);
}

void PhysicsWorld::DrainEvents(IPhysicsEventSink& InSink)
{
	Events.Drain(InSink);
}
} // namespace game::model

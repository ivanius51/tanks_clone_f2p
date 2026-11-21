#include "game/objects/MovableObject.h"

#include <algorithm>
#include <cmath>

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/ObjectsFactory.h"
#include "model/TurnState.h"

namespace tanks
{

namespace
{
	constexpr float UpDegrees = 0.0f;
	constexpr float DownDegrees = 180.0f;
	constexpr float LeftDegrees = 270.0f;
	constexpr float RightDegrees = 90.0f;

	constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;

	// A move is split into steps of at most this many pixels, so that a long frame
	// cannot carry the tank through (or into) a wall.
	constexpr float MaxStep = 8.0f;

	constexpr float SeparationEpsilon = 0.01f;
	// How far a blocked tank looks sideways for a way around a corner, by default, and in what steps.
	constexpr float DefaultCornerAssist = TileSize / 3.0f;
	constexpr float CornerProbeStep = 1.0f;

	constexpr float DefaultTurnTime = 0.12f;
	constexpr float DefaultCounterTurnTime = 0.08f;
	constexpr float DefaultTurnEase = 2.0f;
	constexpr float DefaultTurnAnimationSpeed = 1.5f;
	constexpr float MoveAnimationSpeed = 1.0f;
	// Two facings closer than this are the same facing.
	constexpr float SameFacingDegrees = 1.0f;
	constexpr float MinTurnSpeed = 0.05f;

	bool IsSameFacing(const float InFirst, const float InSecond)
	{
		return std::fabs(model::TurnState::Difference(InFirst, InSecond)) < SameFacingDegrees;
	}

	float OverlapArea(const cocos2d::Rect& InFirst, const cocos2d::Rect& InSecond)
	{
		const float CurrentWidth = std::min(InFirst.getMaxX(), InSecond.getMaxX()) - std::max(InFirst.getMinX(), InSecond.getMinX());
		const float CurrentHeight = std::min(InFirst.getMaxY(), InSecond.getMaxY()) - std::max(InFirst.getMinY(), InSecond.getMinY());
		return CurrentWidth > 0.0f && CurrentHeight > 0.0f ? CurrentWidth * CurrentHeight : 0.0f;
	}
}

bool MovableObject::Build()
{
	const bool bResult = PhysicsObject::Build();
	SetMovable(true);
	StartUpdating();
	return bResult;
}

void MovableObject::Configure(const cocos2d::ValueMap& InData)
{
	PhysicsObject::Configure(InData);
	MoveSpeed = DataConverter::GetFloat(InData, "speed", MoveSpeed);
	CornerAssist = DataConverter::GetFloat(InData, "corner_assist", DefaultCornerAssist);
	TurnTime = DataConverter::GetFloat(InData, "turn_time", DefaultTurnTime);
	CounterTurnTime = DataConverter::GetFloat(InData, "counter_turn_time", DefaultCounterTurnTime);
	TurnAnimationSpeed = DataConverter::GetFloat(InData, "turn_anim_speed", DefaultTurnAnimationSpeed);
	Turn.SetEaseRate(DataConverter::GetFloat(InData, "turn_ease", DefaultTurnEase));
}

const cocos2d::Vec2& MovableObject::GetDirection() const
{
	return Direction;
}

float MovableObject::GetSpeed() const
{
	return MoveSpeed;
}

void MovableObject::SetSpeed(const float InSpeed)
{
	MoveSpeed = InSpeed;
}

bool MovableObject::IsRunning() const
{
	return bIsRunning;
}

bool MovableObject::IsBlocked() const
{
	return bIsBlocked;
}

bool MovableObject::IsTurning() const
{
	return Turn.IsTurning();
}

float MovableObject::GetTurnSpeed() const
{
	return TurnSpeed;
}

void MovableObject::SetTurnSpeed(const float InSpeed)
{
	TurnSpeed = std::max(InSpeed, MinTurnSpeed);
}

float MovableObject::GetTurnTime() const
{
	return TurnTime / TurnSpeed;
}

float MovableObject::GetCounterTurnTime() const
{
	return CounterTurnTime / TurnSpeed;
}

void MovableObject::OnTurnFinished()
{
}

void MovableObject::OnMoveStart()
{
	bIsRunning = true;
}

void MovableObject::OnMoveEnd()
{
	bIsRunning = false;
}

void MovableObject::OnBlocked()
{
	SetAnimationSpeed(0.0f);
}

void MovableObject::UpdateDirection(const float InAngle)
{
	// angle:   0  90  180  270  360
	// sin (x): 0   1    0   -1    0
	// cos (y): 1   0   -1    0    1
	const float Radians = InAngle * DegreesToRadians;
	Direction.x = std::sin(Radians);
	Direction.y = std::cos(Radians);
}

bool MovableObject::CanMoveTo(const cocos2d::Vec2& InPosition) const
{
	const cocos2d::Rect CurrentRect = GetCollisionRectAt(InPosition);
	return IsInsideBounds(CurrentRect) && !FindBlocker(CurrentRect, GetCollisionRect());
}

bool MovableObject::SlideAroundCorner(const float InStep)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( CornerAssist <= 0.0f )
	{
		return false;
	}

	// The nearest sideways shift (up to the assist distance) after which the way ahead is free.
	const cocos2d::Vec2 Position = CurrentVisualNode->getPosition();
	const cocos2d::Vec2 Side(std::fabs(Direction.y), std::fabs(Direction.x));
	for ( float Shift = CornerProbeStep; Shift <= CornerAssist; Shift += CornerProbeStep )
	{
		for ( float Sign : { 1.0f, -1.0f } )
		{
			const cocos2d::Vec2 Shifted = Position + Side * (Sign * Shift);
			if ( !CanMoveTo(Shifted) || !CanMoveTo(Shifted + Direction * InStep) )
			{
				continue;
			}
			const cocos2d::Vec2 Slide = Position + Side * (Sign * std::min(Shift, InStep));
			if ( !CanMoveTo(Slide) )
			{
				return false;
			}
			CurrentVisualNode->setPosition(Slide);
			return true;
		}
	}
	return false;
}

void MovableObject::SeparateFromOthers(const float InDelta)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return;
	}

	const cocos2d::Rect Self = GetCollisionRect();
	const MovableObject* Other = nullptr;
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject(GetType(), [this, &Self, &Other](GameObject& InObject)
	{
		const MovableObject* Movable = ObjectCast<MovableObject>(&InObject);
		if ( !Other && Movable && Movable != this && !Movable->IsDestroying() && !Movable->IsPassable()
			&& Movable->GetVisualNode() && OverlapArea(Self, Movable->GetCollisionRect()) > 0.0f )
		{
			Other = Movable;
		}
	});
	if ( !Other )
	{
		return;
	}

	// Out along the axis where the overlap is smallest, away from the other one's center.
	const cocos2d::Rect Box = Other->GetCollisionRect();
	const float OverlapX = std::min(Self.getMaxX(), Box.getMaxX()) - std::max(Self.getMinX(), Box.getMinX());
	const float OverlapY = std::min(Self.getMaxY(), Box.getMaxY()) - std::max(Self.getMinY(), Box.getMinY());
	const cocos2d::Vec2 Away = CurrentVisualNode->getPosition() - cocos2d::Vec2(Box.getMidX(), Box.getMidY());
	// Exactly on top of each other: the two pick opposite sides.
	const float Tie = this < Other ? -1.0f : 1.0f;
	const float SideX = std::fabs(Away.x) > SeparationEpsilon ? (Away.x > 0.0f ? 1.0f : -1.0f) : Tie;
	const float SideY = std::fabs(Away.y) > SeparationEpsilon ? (Away.y > 0.0f ? 1.0f : -1.0f) : Tie;
	cocos2d::Vec2 Push = OverlapX < OverlapY ? cocos2d::Vec2(SideX, 0.0f) : cocos2d::Vec2(0.0f, SideY);
	Push *= std::min(std::min(OverlapX, OverlapY), MoveSpeed * InDelta);

	const cocos2d::Vec2 CurrentTarget = CurrentVisualNode->getPosition() + Push;
	if ( CanMoveTo(CurrentTarget) )
	{
		CurrentVisualNode->setPosition(CurrentTarget);
	}
}

void MovableObject::Update(const float InDelta)
{
	SeparateFromOthers(InDelta);

	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return;
	}
	if ( Turn.IsTurning() )
	{
		UpdateTurn(InDelta);
		return;
	}
	if ( !bIsRunning )
	{
		return;
	}

	// Move in small steps; a step that hits a corner may slide sideways instead; stop when neither works.
	float CurrentRemaining = InDelta * MoveSpeed;
	bool bBlocked = false;
	while ( CurrentRemaining > 0.0f && !bBlocked )
	{
		const float CurrentStep = std::min(CurrentRemaining, MaxStep);
		const cocos2d::Vec2 CurrentNext = CurrentVisualNode->getPosition() + Direction * CurrentStep;
		if ( CanMoveTo(CurrentNext) )
		{
			CurrentVisualNode->setPosition(CurrentNext);
		}
		else if ( !SlideAroundCorner(CurrentStep) )
		{
			bBlocked = true;
		}
		CurrentRemaining -= CurrentStep;
	}

	if ( !bBlocked )
	{
		if ( bIsBlocked )
		{
			bIsBlocked = false;
			SetAnimationSpeed(MoveAnimationSpeed);
		}
	}
	else
	{
		bIsBlocked = true;
		OnBlocked();
	}
}

void MovableObject::Move()
{
	if ( !GetVisualNode() )
	{
		return;
	}

	if ( !bIsRunning )
	{
		OnMoveStart();
	}
	bIsBlocked = false;
	SetAnimationSpeed(MoveAnimationSpeed);
}

void MovableObject::Run()
{
	bIsRunning = true;
}

void MovableObject::Stop()
{
	if ( bIsRunning )
	{
		OnMoveEnd();
	}
	bMoveAfterTurn = false;
	bIsRunning = false;
	bIsBlocked = false;

	auto CurrentVisualNode = GetVisualNode();
	if ( CurrentVisualNode )
	{
		if ( !Turn.IsTurning() )
		{
			SetAnimationSpeed(0.0f);
		}
		CurrentVisualNode->stopActionByTag(ToTag(ActionTag::Move));
	}
}

bool MovableObject::TurnOn(const float InAngle)
{
	const float Facing = Turn.IsTurning() ? Turn.GetTarget() : GetFacing();
	return TurnTo(Facing + InAngle);
}

bool MovableObject::TurnTo(const float InAngle)
{
	if ( !GetVisualNode() )
	{
		return false;
	}
	if ( Turn.IsTurning() )
	{
		if ( IsSameFacing(InAngle, Turn.GetTarget()) )
		{
			return true;
		}
		if ( IsSameFacing(InAngle, Turn.GetPrevious()) )
		{
			Turn.Reverse(GetCounterTurnTime());
		}
		else
		{
			Turn.Start(Turn.GetAngle(), InAngle, GetTurnTime());
		}
	}
	else
	{
		if ( IsSameFacing(InAngle, GetFacing()) )
		{
			return false;
		}
		Turn.Start(GetFacing(), InAngle, GetTurnTime());
	}
	BeginTurn();
	return true;
}

float MovableObject::GetFacing() const
{
	auto CurrentVisualNode = GetVisualNode();
	return CurrentVisualNode ? model::TurnState::Normalize(CurrentVisualNode->getRotation()) : 0.0f;
}

void MovableObject::BeginTurn()
{
	const bool bKeepMoveAfterTurn = bMoveAfterTurn;
	Stop();
	bMoveAfterTurn = bKeepMoveAfterTurn;
	UpdateDirection(Turn.GetTarget());
	SetAnimationSpeed(TurnAnimationSpeed);
	if ( !Turn.IsTurning() )
	{
		FinishTurn();
	}
}

void MovableObject::UpdateTurn(const float InDelta)
{
	const bool bFinished = Turn.Update(InDelta);
	GetVisualNode()->setRotation(Turn.GetAngle());
	if ( bFinished )
	{
		FinishTurn();
	}
}

void MovableObject::FinishTurn()
{
	GetVisualNode()->setRotation(Turn.GetAngle());
	SetAnimationSpeed(0.0f);
	if ( bMoveAfterTurn )
	{
		bMoveAfterTurn = false;
		Move();
	}
	OnTurnFinished();
}

void MovableObject::SetAnimationSpeed(const float InSpeed)
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return;
	}

	auto CurrentAnimation = CheckedCast<cocos2d::Speed>(CurrentVisualNode->getActionByTag(ToTag(ActionTag::Animation)));
	if ( CurrentAnimation )
	{
		CurrentAnimation->setSpeed(InSpeed);
	}
}

void MovableObject::MoveTowards(const float InAngle)
{
	if ( TurnTo(InAngle) && Turn.IsTurning() )
	{
		bMoveAfterTurn = true;
	}
	else if ( !Turn.IsTurning() )
	{
		Move();
	}
}

void MovableObject::OnCommandReceived(const GameCommand& InCommand)
{
	switch ( InCommand.GetType() )
	{
		case CommandType::MoveForward:
			MoveTowards(UpDegrees);
			break;
		case CommandType::MoveBack:
			MoveTowards(DownDegrees);
			break;
		case CommandType::MoveLeft:
			MoveTowards(LeftDegrees);
			break;
		case CommandType::MoveRight:
			MoveTowards(RightDegrees);
			break;
		case CommandType::Stop:
			Stop();
			break;
		default: // Undefined and Shoot are not handled by a movable object
			break;
	}
}

} // namespace tanks

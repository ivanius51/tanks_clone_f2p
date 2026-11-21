#include "model/ArcTurn.h"

#include <algorithm>

#include "model/TurnState.h"

namespace tanks::model
{

namespace
{
	constexpr float DegreesToRadians = 3.14159265358979323846f / 180.0f;
	constexpr float SameFacingDegrees = 1.0f;
	constexpr float RightAngle = 90.0f;
	constexpr float MinRadius = 1.0f;
}

Vec2f ArcTurn::DirectionOf(const float InAngle)
{
	const float Radians = InAngle * DegreesToRadians;
	return { std::sin(Radians), std::cos(Radians) };
}

void ArcTurn::Start(const Vec2f& InPosition, const float InFrom, const float InTo, const float InRadius)
{
	From = TurnState::Normalize(InFrom);
	Delta = TurnState::Difference(From, TurnState::Normalize(InTo));
	Radius = std::max(InRadius, MinRadius);
	Driven = 0.0f;
	bIsTurning = std::fabs(Delta) >= SameFacingDegrees;
	const float Side = Delta >= 0.0f ? RightAngle : -RightAngle;
	Center = InPosition + DirectionOf(From + Side) * Radius;
}

float ArcTurn::GetLength() const
{
	return std::fabs(Delta) * DegreesToRadians * Radius;
}

Vec2f ArcTurn::PositionAt(const float InTurned) const
{
	const float Side = Delta >= 0.0f ? RightAngle : -RightAngle;
	return Center + DirectionOf(From + InTurned - Side) * Radius;
}

Vec2f ArcTurn::PeekPosition(const float InDistance) const
{
	const float Length = GetLength();
	const float Travelled = std::min(Driven + InDistance, Length);
	return PositionAt(Length > 0.0f ? Delta * Travelled / Length : 0.0f);
}

bool ArcTurn::Advance(const float InDistance)
{
	if ( !bIsTurning )
	{
		return false;
	}
	Driven = std::min(Driven + InDistance, GetLength());
	if ( Driven >= GetLength() )
	{
		bIsTurning = false;
		return true;
	}
	return false;
}

void ArcTurn::Cancel()
{
	bIsTurning = false;
}

bool ArcTurn::IsTurning() const
{
	return bIsTurning;
}

Vec2f ArcTurn::GetPosition() const
{
	return PeekPosition(0.0f);
}

float ArcTurn::GetAngle() const
{
	const float Length = GetLength();
	return TurnState::Normalize(From + (Length > 0.0f ? Delta * Driven / Length : 0.0f));
}

float ArcTurn::GetTarget() const
{
	return TurnState::Normalize(From + Delta);
}

float ArcTurn::GetRemainingDistance() const
{
	return GetLength() - Driven;
}

} // namespace tanks::model

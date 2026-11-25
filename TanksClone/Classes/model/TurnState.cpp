#include "model/TurnState.h"

#include <algorithm>
#include <cmath>

namespace game::model
{

namespace
{
	constexpr float FullCircle = 360.0f;
	constexpr float HalfCircle = 180.0f;
	constexpr float AngleEpsilon = 0.001f;
}

float TurnState::Normalize(const float InAngle)
{
	const float Result = std::fmod(InAngle, FullCircle);
	return Result < 0.0f ? Result + FullCircle : Result;
}

float TurnState::Difference(const float InFrom, const float InTo)
{
	float Result = Normalize(InTo - InFrom);
	if ( Result > HalfCircle )
	{
		Result -= FullCircle;
	}
	return Result;
}

void TurnState::Start(const float InFrom, const float InTo, const float InDuration)
{
	const float Way = Difference(InFrom, InTo);
	Previous = Normalize(InFrom);
	if ( std::fabs(Way) < AngleEpsilon )
	{
		Cancel();
		From = Previous;
		return;
	}
	From = Previous;
	Delta = Way;
	// InDuration is for a quarter turn: a half turn takes twice as long.
	Duration = std::max(InDuration, 0.0f) * std::fabs(Way) / 90.0f;
	Elapsed = 0.0f;
	bIsTurning = true;
	bIsReversed = false;
}

bool TurnState::Reverse(const float InFullDuration)
{
	if ( !bIsTurning )
	{
		return false;
	}
	const float Current = GetAngle();
	const float Target = Previous;
	const float Done = std::fabs(Difference(Current, Target)) / std::max(std::fabs(Delta), AngleEpsilon);
	Previous = Normalize(From + Delta);
	From = Current;
	Delta = Difference(Current, Target);
	Duration = std::max(InFullDuration, 0.0f) * std::min(Done, 1.0f);
	Elapsed = 0.0f;
	bIsReversed = true;
	if ( std::fabs(Delta) < AngleEpsilon )
	{
		From = Target;
		Delta = 0.0f;
		bIsTurning = false;
	}
	return true;
}

bool TurnState::Update(const float InDelta)
{
	if ( !bIsTurning )
	{
		return false;
	}
	Elapsed += std::max(InDelta, 0.0f);
	if ( Elapsed < Duration )
	{
		return false;
	}
	From = Normalize(From + Delta);
	Delta = 0.0f;
	bIsTurning = false;
	return true;
}

void TurnState::Cancel()
{
	From = GetAngle();
	Delta = 0.0f;
	bIsTurning = false;
	bIsReversed = false;
}

bool TurnState::IsTurning() const
{
	return bIsTurning;
}

bool TurnState::IsReversed() const
{
	return bIsReversed;
}

float TurnState::GetAngle() const
{
	return Normalize(From + Delta * GetProgress());
}

float TurnState::GetTarget() const
{
	return Normalize(From + Delta);
}

float TurnState::GetPrevious() const
{
	return Previous;
}

void TurnState::SetEaseRate(const float InRate)
{
	EaseRate = std::max(InRate, 1.0f);
}

float TurnState::GetProgress() const
{
	if ( !bIsTurning || Duration <= 0.0f )
	{
		return bIsTurning ? 1.0f : 0.0f;
	}
	const float Linear = std::min(Elapsed / Duration, 1.0f);
	return 1.0f - std::pow(1.0f - Linear, EaseRate);
}

} // namespace game::model

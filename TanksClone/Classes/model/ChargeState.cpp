#include "model/ChargeState.h"

#include <algorithm>

namespace tanks::model
{

void ChargeState::Configure(const float InChargeTime, const float InStunTime, const float InCooldown)
{
	ChargeTime = std::max(0.0f, InChargeTime);
	StunTime = std::max(0.0f, InStunTime);
	Cooldown = std::max(0.0f, InCooldown);
}

void ChargeState::Enter(const Phase InPhase)
{
	Current = InPhase;
	switch ( InPhase )
	{
		case Phase::Charging:
			Timer = ChargeTime;
			break;
		case Phase::Stunned:
			Timer = StunTime;
			break;
		case Phase::Cooldown:
			Timer = Cooldown;
			break;
		case Phase::Ready:
			Timer = 0.0f;
			break;
	}
}

bool ChargeState::Start()
{
	if ( Current != Phase::Ready )
	{
		return false;
	}
	Enter(Phase::Charging);
	return true;
}

void ChargeState::Hit()
{
	if ( Current == Phase::Charging )
	{
		Enter(Phase::Stunned);
	}
}

bool ChargeState::Update(const float InDelta)
{
	if ( Current == Phase::Ready )
	{
		return false;
	}
	Timer -= InDelta;
	if ( Timer > 0.0f )
	{
		return false;
	}
	switch ( Current )
	{
		case Phase::Charging:
			Enter(Phase::Stunned);
			break;
		case Phase::Stunned:
			Enter(Phase::Cooldown);
			break;
		default:
			Enter(Phase::Ready);
			break;
	}
	return true;
}

ChargeState::Phase ChargeState::GetPhase() const
{
	return Current;
}

bool ChargeState::IsCharging() const
{
	return Current == Phase::Charging;
}

bool ChargeState::IsStunned() const
{
	return Current == Phase::Stunned;
}

float ChargeState::GetReadiness() const
{
	const float Total = ChargeTime + StunTime + Cooldown;
	if ( Current == Phase::Ready || Total <= 0.0f )
	{
		return 1.0f;
	}
	float Left = Timer;
	if ( Current == Phase::Charging )
	{
		Left += StunTime + Cooldown;
	}
	else if ( Current == Phase::Stunned )
	{
		Left += Cooldown;
	}
	return 1.0f - std::min(1.0f, Left / Total);
}

float ChargeState::GetRemaining() const
{
	return Current == Phase::Ready ? 0.0f : Timer;
}

} // namespace tanks::model

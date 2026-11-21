#pragma once

namespace tanks::model
{

/**
 * The phases of a ram charge: ready, charging for a limited time, stunned after it ends (whatever ended it),
 * then a cooldown before the next charge. Engine free, so it is unit-tested.
 */
class ChargeState
{
public:
	enum class Phase
	{
		Ready,
		Charging,
		Stunned,
		Cooldown
	};

	void Configure(float InChargeTime, float InStunTime, float InCooldown);
	// False unless ready.
	bool Start();
	// Ends the charge early (it hit something): the stun starts now.
	void Hit();
	// Advances the timers. True on the step the phase changes.
	bool Update(float InDelta);

	Phase GetPhase() const;
	bool IsCharging() const;
	bool IsStunned() const;
	// 0 = just started charging, 1 = ready again.
	float GetReadiness() const;
	float GetRemaining() const;

private:
	void Enter(Phase InPhase);

	Phase Current = Phase::Ready;
	float Timer = 0.0f;
	float ChargeTime = 1.0f;
	float StunTime = 1.0f;
	float Cooldown = 3.0f;
};

} // namespace tanks::model

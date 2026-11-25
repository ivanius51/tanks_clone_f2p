#pragma once

namespace game::model
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
	bool Start();
	void Hit();
	bool Update(float InDelta);

	Phase GetPhase() const;
	bool IsCharging() const;
	bool IsStunned() const;
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
} // namespace game::model

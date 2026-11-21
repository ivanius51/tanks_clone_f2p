#pragma once

namespace tanks::model
{

/**
 * A shell lobbed over everything: it flies a set distance along the ground, rising and falling on the way,
 * and lands at the end. Nothing on the way stops it.
 */
class LobFlight
{
public:
	void Start(float InDistance);
	// Moves InStep further. True on the step it lands.
	bool Advance(float InStep);

	bool IsFlying() const;
	// 0 at the start, 1 on landing.
	float GetProgress() const;
	float GetDistance() const;
	// 0 on the ground, 1 at the top of the arc, half way.
	float GetHeight() const;

	// How far to lob: the distance to a target ahead (0 or less = none) kept within [InMin, InMax]; no target = InMax.
	static float ChooseDistance(float InTargetDistance, float InMin, float InMax);

private:
	float Distance = 0.0f;
	float Travelled = 0.0f;
	bool bFlying = false;
};

} // namespace tanks::model

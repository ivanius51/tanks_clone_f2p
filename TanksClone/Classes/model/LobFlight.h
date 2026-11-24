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
	bool Advance(float InStep);

	bool IsFlying() const;
	float GetProgress() const;
	float GetDistance() const;
	float GetHeight() const;

	static float ChooseDistance(float InTargetDistance, float InMin, float InMax);

private:
	float Distance = 0.0f;
	float Travelled = 0.0f;
	bool bFlying = false;
};
} // namespace tanks::model

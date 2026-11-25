#pragma once

#include "model/MathTypes.h"

namespace game::model
{
/**
 * A wheeled vehicle turning while it drives: it follows a circular arc of a set radius from one facing to another
 * (degrees, 0 = up, 90 = right) instead of rotating on the spot. A U-turn is a half circle.
 */
class ArcTurn
{
public:
	void Start(const Vec2f& InPosition, float InFrom, float InTo, float InRadius);
	Vec2f PeekPosition(float InDistance) const;
	bool Advance(float InDistance);
	void Cancel();

	bool IsTurning() const;
	Vec2f GetPosition() const;
	float GetAngle() const;
	float GetTarget() const;
	float GetRemainingDistance() const;

	static Vec2f DirectionOf(float InAngle);

private:
	Vec2f PositionAt(float InTurned) const;
	float GetLength() const;

	Vec2f Center;
	float From = 0.0f;
	float Delta = 0.0f;
	float Radius = 0.0f;
	float Driven = 0.0f;
	bool bIsTurning = false;
};
} // namespace game::model

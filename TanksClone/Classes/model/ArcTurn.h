#pragma once

#include "model/MathTypes.h"

namespace tanks::model
{

/**
 * A wheeled vehicle turning while it drives: it follows a circular arc of a set radius from one facing to another
 * (degrees, 0 = up, 90 = right) instead of rotating on the spot. A U-turn is a half circle.
 */
class ArcTurn
{
public:
	// Starts at InPosition facing InFrom, turning the shorter way to InTo. A target equal to the facing starts nothing.
	void Start(const Vec2f& InPosition, float InFrom, float InTo, float InRadius);
	// Where the vehicle would be after driving InDistance more along the arc (clamped to its end), without moving it.
	Vec2f PeekPosition(float InDistance) const;
	// Drives InDistance along the arc. True on the step it finishes.
	bool Advance(float InDistance);
	void Cancel();

	bool IsTurning() const;
	Vec2f GetPosition() const;
	float GetAngle() const;
	float GetTarget() const;
	float GetRemainingDistance() const;

	// Unit vector of a facing: 0 = up (0, 1), 90 = right (1, 0).
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

} // namespace tanks::model

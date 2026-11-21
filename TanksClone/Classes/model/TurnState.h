#pragma once

namespace tanks::model
{

/**
 * A vehicle turning between two facings (degrees, 0 = up, 90 = right) with an ease-out over a set time.
 * A turn can be reversed midway (counter-turn): it goes back from the current angle to the facing it started from.
 */
class TurnState
{
public:
	// Turns along the shorter way. A target equal to the current facing starts nothing.
	void Start(float InFrom, float InTo, float InDuration);
	// Back to the facing the turn started from, from the current angle. InFullDuration is the time of a whole
	// counter-turn; a turn reversed early takes only the part it has to undo. False if not turning.
	bool Reverse(float InFullDuration);
	// Advances the turn. True on the step it finishes.
	bool Update(float InDelta);
	void Cancel();

	bool IsTurning() const;
	bool IsReversed() const;
	float GetAngle() const;
	float GetTarget() const;
	float GetPrevious() const;
	// Ease-out rate: 1 is linear, larger starts faster and slows down at the end.
	void SetEaseRate(float InRate);

	// InAngle in [0, 360).
	static float Normalize(float InAngle);
	// Shortest signed way from InFrom to InTo, in (-180, 180].
	static float Difference(float InFrom, float InTo);

private:
	float GetProgress() const;

	float From = 0.0f;
	float Delta = 0.0f;
	float Previous = 0.0f;
	float Duration = 0.0f;
	float Elapsed = 0.0f;
	float EaseRate = 2.0f;
	bool bIsTurning = false;
	bool bIsReversed = false;
};

} // namespace tanks::model

#pragma once

namespace game::model
{
/**
 * A vehicle turning between two facings (degrees, 0 = up, 90 = right) with an ease-out over a set time.
 * A turn can be reversed midway (counter-turn): it goes back from the current angle to the facing it started from.
 */
class TurnState
{
public:
	void Start(float InFrom, float InTo, float InDuration);
	bool Reverse(float InFullDuration);
	bool Update(float InDelta);
	void Cancel();

	bool IsTurning() const;
	bool IsReversed() const;
	float GetAngle() const;
	float GetTarget() const;
	float GetPrevious() const;
	void SetEaseRate(float InRate);

	static float Normalize(float InAngle);
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
} // namespace game::model

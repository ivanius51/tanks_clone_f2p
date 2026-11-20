#pragma once

#include "game/objects/PhysicsObject.h"

namespace tanks
{

class MovableObject : public PhysicsObject
{
public:
	bool Build() override;
	void Configure(const cocos2d::ValueMap& InData) override;

	void Move();
	void Run();
	void Stop();
	// Both return the duration of the rotation, or 0 if no rotation was started.
	float TurnOn(float InAngle);
	float TurnTo(float InAngle);

	// Unit vector the object is facing (already updated while it is still turning).
	const cocos2d::Vec2& GetDirection() const;
	float GetSpeed() const;
	void SetSpeed(float InSpeed);
	bool IsRunning() const;
	bool IsBlocked() const;

	void OnCommandReceived(const GameCommand& InCommand) override;

	// True if the object may stand at InPosition: inside the bounds, and not overlapping anything solid more
	// than it already does (so objects stuck inside each other can still move apart).
	bool CanMoveTo(const cocos2d::Vec2& InPosition) const;

protected:
	virtual void OnMoveStart();
	virtual void OnMoveEnd();
	// Called every frame the object tries to move but something is in the way.
	virtual void OnBlocked();
	void UpdateDirection(float InAngle);
	void Update(float InDelta) override;

	// Turns to the given angle (0 = up, 90 = right...) and then starts moving.
	void MoveTowards(float InAngle);

private:
	// Recomputes the direction. Returns true if it actually changed.
	bool ApplyDirection(float InAngle);
	// A forward step of InStep is blocked: if a sideways shift of at most "corner_assist" pixels clears the way,
	// moves one step sideways towards it (a tank catching a corner slides off it). Turning never moves the tank.
	bool SlideAroundCorner(float InStep);
	// Overlapping another movable object of the same type: steps out along the axis of the smaller overlap,
	// no faster than driving and only where it is free (never into a wall).
	void SeparateFromOthers(float InDelta);
	// Stops the vehicle and runs the given rotation. Returns its duration.
	float RunRotation(cocos2d::ActionInterval* InRotation);
	void SetAnimationSpeed(float InSpeed);

	bool bIsRunning = false;
	bool bIsRotating = false;
	bool bIsBlocked = false;
	float MoveSpeed = 100.0f;
	float CornerAssist = 0.0f;

	cocos2d::Vec2 Direction = cocos2d::Vec2(0.0f, 1.0f);
};

} // namespace tanks

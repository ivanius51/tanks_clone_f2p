#pragma once

#include "game/objects/PhysicsObject.h"
#include "model/ArcTurn.h"
#include "model/TurnState.h"

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
	bool TurnOn(float InAngle);
	bool TurnTo(float InAngle);
	bool IsTurning() const;
	float GetTurnSpeed() const;
	void SetTurnSpeed(float InSpeed);

	const cocos2d::Vec2& GetDirection() const;
	float GetSpeed() const;
	void SetSpeed(float InSpeed);
	bool IsRunning() const;
	bool IsWheeled() const { return bWheeled; }
	bool IsBlocked() const;

	void OnCommandReceived(const GameCommand& InCommand) override;

	bool CanMoveTo(const cocos2d::Vec2& InPosition) const;

protected:
	virtual void OnMoveStart();
	virtual void OnMoveEnd();
	virtual void OnBlocked();
	virtual void OnTurnFinished();
	void UpdateDirection(float InAngle);
	void Update(float InDelta) override;

	void MoveTowards(float InAngle);

private:
	float GetFacing() const;
	float GetTurnTime() const;
	float GetCounterTurnTime() const;
	void StartArc(float InAngle);
	void UpdateArc(float InDelta);
	void FinishArc();
	void BeginTurn();
	void UpdateTurn(float InDelta);
	void FinishTurn();
	bool SlideAroundCorner(float InStep);
	void SeparateFromOthers(float InDelta);
	void SetAnimationSpeed(float InSpeed);
	void SetTrackSpeeds(float InLeft, float InRight);

	bool bIsRunning = false;
	bool bMoveAfterTurn = false;
	bool bIsBlocked = false;
	float MoveSpeed = 100.0f;
	float CornerAssist = 0.0f;
	float TurnTime = 0.0f;
	float CounterTurnTime = 0.0f;
	float TurnAnimationSpeed = 0.0f;
	float TurnSpeed = 1.0f;
	float TrackReferenceSpeed = 100.0f;
	model::TurnState Turn;
	bool bWheeled = false;
	float TurnRadius = 0.0f;
	model::ArcTurn Arc;

	cocos2d::Vec2 Direction = cocos2d::Vec2(0.0f, 1.0f);
};
} // namespace tanks

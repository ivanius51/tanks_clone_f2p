#pragma once

#include "model/MathTypes.h"
#include "templates/TContainers.h"

namespace game::model
{
/** A cell of a wall mask: column 0 on the left, row 0 at the bottom. */
struct CellCoord
{
	int Column = 0;
	int Row = 0;
};

/** What one hit did to a wall: how many cells fell, which ones, and the direction the shell was travelling. */
struct WallChipResult
{
	int Removed = 0;
	Vec2f Heading;
	TArray<CellCoord> BrokenCells;
};

/** A wall was chipped; Contact is the world point of the hit and bDestroyed says nothing is left standing. */
struct WallChipEvent
{
	unsigned BodyId = 0;
	Vec2f Contact;
	WallChipResult Result;
	bool bDestroyed = false;
};

/** A projectile hit something; the view shows hit particles only when the target is still standing. */
struct BulletHitEvent
{
	unsigned BulletId = 0;
	unsigned TargetId = 0;
	Vec2f Point;
	Vec2f Direction;
	bool bTargetDestroyed = false;
};

/** Receives what the physics did during a step. The view implements it to show particles, debris and sounds. */
class IPhysicsEventSink
{
public:
	virtual ~IPhysicsEventSink() = default;

	virtual void OnWallChipped(const WallChipEvent& InEvent) = 0;
	virtual void OnBulletHit(const BulletHitEvent& InEvent) = 0;
};

/** Collects events during a step and hands them over in order afterwards, so the physics may run on another thread. */
class PhysicsEventQueue final : public IPhysicsEventSink
{
public:
	void OnWallChipped(const WallChipEvent& InEvent) override;
	void OnBulletHit(const BulletHitEvent& InEvent) override;

	/** Plays the events to a sink in the order they happened and empties the queue. */
	void Drain(IPhysicsEventSink& InSink);
	bool IsEmpty() const;

private:
	enum class EKind
	{
		Chip,
		Hit
	};

	TArray<EKind> Order;
	TArray<WallChipEvent> Chips;
	TArray<BulletHitEvent> Hits;
};
} // namespace game::model

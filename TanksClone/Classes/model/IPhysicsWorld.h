#pragma once

#include "model/CellGrid.h"
#include "model/PhysicsEvents.h"
#include "model/PhysicsTypes.h"

namespace game::model
{
/**
 * The seam between the game and a physics backend (the built-in simulation, a prebuilt library, a worker thread).
 * The game speaks only of bodies, shapes, collision profiles, sweeps and contacts, named after Unreal's physics
 * so another backend maps onto them. Step advances the world by a fixed time and only writes events; the caller
 * reads them with DrainEvents after Step returned, never during it, so a backend may step on its own thread.
 */
class IPhysicsWorld
{
public:
	virtual ~IPhysicsWorld() = default;

	virtual BodyId CreateBody(const BodyDef& InDefinition) = 0;
	virtual void DestroyBody(BodyId InBody) = 0;
	virtual Vec2f GetPosition(BodyId InBody) const = 0;
	virtual void SetPosition(BodyId InBody, const Vec2f& InPosition) = 0;
	virtual void SetVelocity(BodyId InBody, const Vec2f& InVelocity) = 0;

	/** Moves a shape from one center to another and reports the first body that blocks it. */
	virtual bool SweepSingle(const Vec2f& InStart, const Vec2f& InEnd, const Shape& InShape, const CollisionProfile& InProfile, HitResult& OutHit) const = 0;

	/** Removes cells from a grid body where an impact came from, and queues a CarveEvent. */
	virtual GridCarveResult Carve(BodyId InBody, const RectF& InArea, HitSide InSide, int InDepthQuarters, int InDamage) = 0;

	virtual void Step(float InDeltaSeconds) = 0;
	virtual void DrainEvents(IPhysicsEventSink& InSink) = 0;
};
} // namespace game::model

#pragma once

#include "model/PhysicsEvents.h"

namespace game::model
{
/**
 * The seam between the game and a physics backend (the built-in simulation, a prebuilt library, a worker thread).
 * Step advances the world by a fixed time and only writes the events it produces; the caller reads them with
 * DrainEvents after Step returned, never during it, so a backend may step on its own thread. Game code gives
 * commands and reads results through this interface and never touches a backend's own types.
 */
class IPhysicsWorld
{
public:
	virtual ~IPhysicsWorld() = default;

	virtual void Step(float InDeltaSeconds) = 0;
	virtual void DrainEvents(IPhysicsEventSink& InSink) = 0;
};
} // namespace game::model

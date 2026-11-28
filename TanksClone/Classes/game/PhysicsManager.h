#pragma once

#include <utility>

#include "model/PhysicsWorld.h"
#include "templates/TContainers.h"
#include "templates/TTemplateHelper.h"

namespace game
{
class PhysicsObject;

/** Something that reacts to what happened to its body: a wall showing debris when cells are carved away. */
class IBodyListener
{
public:
	virtual ~IBodyListener() = default;

	virtual void OnCarved([[maybe_unused]] const model::CarveEvent& InEvent) {}
	virtual void OnContact([[maybe_unused]] const model::ContactEvent& InEvent) {}
};

/**
 * Owns the physics world of the running level and passes its events to the objects that stand for bodies.
 * Pump drains the world and must be called after the physics stepped or was commanded; in this single-thread
 * setup the callers pump right after a command, a threaded backend would pump once per frame.
 */
class PhysicsManager final : public FNonCopyableNonMovable, private model::IPhysicsEventSink
{
public:
	model::IPhysicsWorld& GetWorld() { return World; }
	const model::IPhysicsWorld& GetWorld() const { return World; }

	void Listen(model::BodyId InBody, IBodyListener* InListener);
	void Forget(model::BodyId InBody);
	void Pump();

	/** Starts a new frame, so the next SyncBodies call does the work again. */
	void BeginFrame() { ++Frame; }

	/** Copies every object that has a body into the world once per frame; later calls in the same frame do nothing. */
	void SyncBodies();

	void Bind(model::BodyId InBody, PhysicsObject* InObject);
	PhysicsObject* FindObject(model::BodyId InBody) const;

private:
	void OnContact(const model::ContactEvent& InEvent) override;
	void OnCarved(const model::CarveEvent& InEvent) override;

	model::PhysicsWorld World;
	TArray<std::pair<model::BodyId, IBodyListener*>> Listeners;
	TArray<PhysicsObject*> Objects;
	unsigned Frame = 1;
	unsigned SyncedFrame = 0;
	bool bPumping = false;
	bool bPumpAgain = false;
};
} // namespace game

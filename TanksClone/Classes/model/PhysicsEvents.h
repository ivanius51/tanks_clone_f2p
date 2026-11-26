#pragma once

#include "model/MathTypes.h"
#include "model/PhysicsTypes.h"
#include "templates/TContainers.h"

namespace game::model
{
/** A cell of a grid shape: column 0 on the left, row 0 at the bottom. */
struct CellCoord
{
	int Column = 0;
	int Row = 0;
};

/** What one impact did to a grid shape: how many cells fell, which ones, and the direction the impact travelled. */
struct GridCarveResult
{
	int Removed = 0;
	Vec2f Heading;
	TArray<CellCoord> BrokenCells;
};

/** Two bodies met. Self is the one that moved; Hit holds the detail (Hit.Item is the other body). */
struct ContactEvent
{
	ContactKind Kind = ContactKind::Hit;
	BodyId Self;
	unsigned SelfTag = 0;
	unsigned OtherTag = 0;
	HitResult Hit;
};

/** Cells were carved out of a grid body; bEmptied says no cell is left. */
struct CarveEvent
{
	BodyId Body;
	unsigned BodyTag = 0;
	Vec2f Point;
	GridCarveResult Result;
	bool bEmptied = false;
};

/** Receives what the physics did during a step. The view implements it to show particles, debris and sounds. */
class IPhysicsEventSink
{
public:
	virtual ~IPhysicsEventSink() = default;

	virtual void OnContact(const ContactEvent& InEvent) = 0;
	virtual void OnCarved(const CarveEvent& InEvent) = 0;
};

/** Collects events during a step and hands them over in order afterwards, so the physics may run on another thread. */
class PhysicsEventQueue final : public IPhysicsEventSink
{
public:
	void OnContact(const ContactEvent& InEvent) override;
	void OnCarved(const CarveEvent& InEvent) override;

	/** Plays the events to a sink in the order they happened and empties the queue. */
	void Drain(IPhysicsEventSink& InSink);
	bool IsEmpty() const;

private:
	enum class EKind
	{
		Contact,
		Carve
	};

	TArray<EKind> Order;
	TArray<ContactEvent> Contacts;
	TArray<CarveEvent> Carves;
};
} // namespace game::model

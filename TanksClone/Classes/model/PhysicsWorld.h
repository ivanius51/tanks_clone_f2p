#pragma once

#include <optional>

#include "model/GridBody.h"
#include "model/IPhysicsWorld.h"
#include "templates/TContainers.h"

namespace game::model
{
/**
 * The built-in physics world: static, dynamic and kinematic bodies with box, rounded box, circle and grid
 * shapes, swept against each other by collision profile. Shapes are axis aligned boxes for now (a circle is its
 * bounding box). A dynamic body stops at the first blocking hit and a ContactEvent is queued. Bodies are stepped
 * in creation order, so the result is deterministic.
 */
class PhysicsWorld final : public IPhysicsWorld
{
public:
	BodyId CreateBody(const BodyDef& InDefinition) override;
	void DestroyBody(BodyId InBody) override;
	Vec2f GetPosition(BodyId InBody) const override;
	void SetPosition(BodyId InBody, const Vec2f& InPosition) override;
	void SetVelocity(BodyId InBody, const Vec2f& InVelocity) override;
	void SetBodyFilter(BodyId InBody, CollisionChannel InObjectType, unsigned InGroup) override;
	void SetEnabled(BodyId InBody, bool bInEnabled) override;
	void StepBody(BodyId InBody, float InDeltaSeconds) override;
	bool SweepSingle(const Vec2f& InStart, const Vec2f& InEnd, const Shape& InShape, const CollisionProfile& InProfile, HitResult& OutHit) const override;
	void ReportContact(const ContactEvent& InEvent) override;
	bool SweepFiltered(const Vec2f& InStart, const Vec2f& InEnd, const Vec2f& InHalf, TFunctionRef<bool(BodyId)> InAccept, HitResult& OutHit) const override;
	const CellGrid* GetGrid(BodyId InBody) const override;
	bool TestOverlap(BodyId InBody, const RectF& InArea) const override;
	bool SweepBody(BodyId InBody, const Vec2f& InStart, const Vec2f& InEnd, const Vec2f& InHalf, float& OutTime) const override;
	GridCarveResult Carve(BodyId InBody, const RectF& InArea, HitSide InSide, int InDepthQuarters, int InDamage) override;
	void Step(float InDeltaSeconds) override;
	void DrainEvents(IPhysicsEventSink& InSink) override;

private:
	struct Body
	{
		BodyDef Definition;
		Vec2f Velocity;
		bool bAlive = true;
		bool bEnabled = true;
		std::optional<GridBody> Grid;
	};

	Body* Find(BodyId InBody);
	const Body* Find(BodyId InBody) const;
	static RectF GetBox(const Body& InBody);
	bool Sweep(const Vec2f& InStart, const Vec2f& InEnd, const Shape& InShape, const BodyDef& InMover, BodyId InIgnored, HitResult& OutHit) const;
	void Advance(size_t InIndex, float InDeltaSeconds);
	static bool IsFilteredOut(const BodyDef& InMover, const Body& InOther);

	TArray<Body> Bodies;
	TArray<unsigned> FreeSlots;
	PhysicsEventQueue Events;
};
} // namespace game::model

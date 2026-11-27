#include "tests/CollisionTests.h"

#include "model/Collision.h"
#include "model/PhysicsEvents.h"
#include "model/PhysicsWorld.h"
#include "model/RenderLayer.h"
#include "model/GridBody.h"

namespace game
{
namespace tests
{

void AddCollisionTests(TestSuite& InSuite)
{
	InSuite.Add("Collision: a vehicle box is squared and a quarter turn swaps the sides", [](TestContext& InContext)
	{
		const model::RectF Plain = model::collision::MakeBodyRect({ 10.0f, 10.0f }, { 20.0f, 40.0f }, { 1.0f, 1.0f }, 0.0f, false);
		InContext.ExpectNear(Plain.Size.X, 20.0f, 0.001f, "plain width");
		InContext.ExpectNear(Plain.Size.Y, 40.0f, 0.001f, "plain height");
		const model::RectF Turned = model::collision::MakeBodyRect({ 10.0f, 10.0f }, { 20.0f, 40.0f }, { 1.0f, 1.0f }, 90.0f, false);
		InContext.ExpectNear(Turned.Size.X, 40.0f, 0.001f, "turned width");
		const model::RectF Vehicle = model::collision::MakeBodyRect({ 10.0f, 10.0f }, { 20.0f, 40.0f }, { 1.0f, 1.0f }, 0.0f, true);
		InContext.ExpectNear(Vehicle.Size.X, 40.0f, 0.001f, "vehicle width is the larger side");
		InContext.ExpectNear(Vehicle.GetCenter().X, 10.0f, 0.001f, "the center stays put");
	});

	InSuite.Add("Collision: overlap area and bounds", [](TestContext& InContext)
	{
		const model::RectF First(0.0f, 0.0f, 10.0f, 10.0f);
		InContext.ExpectNear(model::collision::OverlapArea(First, model::RectF(5.0f, 5.0f, 10.0f, 10.0f)), 25.0f, 0.001f, "quarter overlap");
		InContext.ExpectNear(model::collision::OverlapArea(First, model::RectF(10.0f, 0.0f, 5.0f, 5.0f)), 0.0f, 0.001f, "touching sides do not overlap");
		InContext.Expect(model::collision::IsInside(First, model::RectF(1.0f, 1.0f, 8.0f, 8.0f)), "inside");
		InContext.Expect(!model::collision::IsInside(First, model::RectF(5.0f, 5.0f, 8.0f, 8.0f)), "sticking out");
	});

	InSuite.Add("Collision: a sweep stops at the box", [](TestContext& InContext)
	{
		const model::RectF Box(10.0f, 0.0f, 10.0f, 10.0f);
		float Fraction = 0.0f;
		InContext.Expect(model::collision::SweepBox(Box, { 0.0f, 5.0f }, { 20.0f, 5.0f }, { 1.0f, 1.0f }, Fraction), "the sweep hits");
		InContext.ExpectNear(Fraction, 0.45f, 0.001f, "it enters at x = 9");
		InContext.Expect(!model::collision::SweepBox(Box, { 0.0f, 50.0f }, { 20.0f, 50.0f }, { 1.0f, 1.0f }, Fraction), "a parallel sweep apart misses");
	});

	InSuite.Add("RenderLayers: the default order runs back to front and keys find their layer", [](TestContext& InContext)
	{
		const model::RenderLayers Layers;
		for ( int Index = 1; Index < model::RenderLayerCount; ++Index )
		{
			InContext.Expect(Layers.GetOrder(static_cast<model::RenderLayer>(Index)) > Layers.GetOrder(static_cast<model::RenderLayer>(Index - 1)), "each layer is in front of the one before");
		}
		model::RenderLayer Found = model::RenderLayer::Floor;
		InContext.Expect(model::FindRenderLayer("tanks", Found) && Found == model::RenderLayer::Tanks, "the tanks key");
		InContext.Expect(!model::FindRenderLayer("nonsense", Found), "an unknown key is refused");
		InContext.Expect(Layers.GetOrder(model::RenderLayer::Debris) < Layers.GetOrder(model::RenderLayer::Tanks) && Layers.GetOrder(model::RenderLayer::Walls) < Layers.GetOrder(model::RenderLayer::Debris), "debris lies above walls and under tanks");
	});

	InSuite.Add("Physics world: a reported contact and a filtered sweep", [](TestContext& InContext)
	{
		model::PhysicsWorld World;
		model::BodyDef Definition;
		Definition.Type = model::BodyType::Kinematic;
		Definition.BodyShape.Kind = model::ShapeKind::Box;
		Definition.BodyShape.Size = model::Vec2f(10.0f, 10.0f);
		Definition.BodyShape.Anchor = model::Vec2f(0.0f, 0.0f);
		Definition.Position = model::Vec2f(100.0f, 0.0f);
		const model::BodyId Near = World.CreateBody(Definition);
		Definition.Position = model::Vec2f(200.0f, 0.0f);
		const model::BodyId Far = World.CreateBody(Definition);
		model::HitResult Hit;
		const bool bAll = World.SweepFiltered(model::Vec2f(0.0f, 5.0f), model::Vec2f(300.0f, 5.0f), model::Vec2f(1.0f, 1.0f), [](model::BodyId) { return true; }, Hit);
		InContext.Expect(bAll && Hit.Item == Near, "the nearest body is hit first");
		const bool bSkipped = World.SweepFiltered(model::Vec2f(0.0f, 5.0f), model::Vec2f(300.0f, 5.0f), model::Vec2f(1.0f, 1.0f), [Near](model::BodyId InBody) { return InBody != Near; }, Hit);
		InContext.Expect(bSkipped && Hit.Item == Far, "the filter skips a body");

		struct Counter final : model::IPhysicsEventSink
		{
			int Contacts = 0;
			void OnCarved(const model::CarveEvent&) override {}
			void OnContact(const model::ContactEvent&) override { ++Contacts; }
		};
		model::ContactEvent Contact;
		Contact.Self = Near;
		World.ReportContact(Contact);
		Counter Sink;
		World.DrainEvents(Sink);
		InContext.ExpectEqual(Sink.Contacts, 1, "a reported contact is delivered");
	});

	InSuite.Add("Physics world: a sweep from the shooter meets a touching wall at its surface", [](TestContext& InContext)
	{
		model::PhysicsWorld World;
		model::BodyDef Definition;
		Definition.Type = model::BodyType::Static;
		Definition.BodyShape.Kind = model::ShapeKind::Grid;
		Definition.BodyShape.Size = model::Vec2f(64.0f, 64.0f);
		Definition.BodyShape.Anchor = model::Vec2f(0.0f, 0.0f);
		Definition.BodyShape.GridCellsPerSide = 8;
		Definition.Position = model::Vec2f(100.0f, 0.0f);
		World.CreateBody(Definition);
		model::HitResult Hit;
		const model::Vec2f Half(3.0f, 3.0f);
		const bool bFromShooter = World.SweepFiltered(model::Vec2f(70.0f, 32.0f), model::Vec2f(102.0f, 32.0f), Half, [](model::BodyId) { return true; }, Hit);
		InContext.Expect(bFromShooter, "the wall is hit");
		InContext.ExpectNear(Hit.Location.X, 97.0f, 0.1f, "the shell stops with its front edge on the wall surface");
	});

	InSuite.Add("Physics world: a stepped projectile respects groups, the ignore point, disabling and slot reuse", [](TestContext& InContext)
	{
		model::PhysicsWorld World;
		model::BodyDef Wall;
		Wall.Type = model::BodyType::Static;
		Wall.BodyShape.Kind = model::ShapeKind::Box;
		Wall.BodyShape.Size = model::Vec2f(10.0f, 10.0f);
		Wall.BodyShape.Anchor = model::Vec2f(0.0f, 0.0f);
		Wall.Position = model::Vec2f(100.0f, 0.0f);
		Wall.Group = 1;
		const model::BodyId Target = World.CreateBody(Wall);

		model::BodyDef Shell;
		Shell.Type = model::BodyType::Dynamic;
		Shell.BodyShape.Kind = model::ShapeKind::Box;
		Shell.BodyShape.Size = model::Vec2f(4.0f, 4.0f);
		Shell.BodyShape.Anchor = model::Vec2f(0.5f, 0.5f);
		Shell.Profile.ObjectType = model::CollisionChannel::Projectile;
		Shell.Position = model::Vec2f(50.0f, 5.0f);
		Shell.Group = 2;
		const model::BodyId Flying = World.CreateBody(Shell);
		World.SetVelocity(Flying, model::Vec2f(100.0f, 0.0f));
		World.StepBody(Flying, 1.0f);
		InContext.ExpectNear(World.GetPosition(Flying).X, 98.0f, 0.1f, "a shell of another group stops at the target");

		World.SetPosition(Flying, model::Vec2f(50.0f, 5.0f));
		World.SetVelocity(Flying, model::Vec2f(100.0f, 0.0f));
		World.SetEnabled(Target, false);
		World.StepBody(Flying, 1.0f);
		InContext.ExpectNear(World.GetPosition(Flying).X, 150.0f, 0.1f, "a disabled body does not block");

		World.SetEnabled(Target, true);
		World.SetBodyFilter(Target, model::CollisionChannel::Pawn, 2);
		World.SetPosition(Flying, model::Vec2f(50.0f, 5.0f));
		World.SetVelocity(Flying, model::Vec2f(100.0f, 0.0f));
		World.StepBody(Flying, 1.0f);
		InContext.ExpectNear(World.GetPosition(Flying).X, 150.0f, 0.1f, "the same group is passed through");

		World.DestroyBody(Target);
		const model::BodyId Reused = World.CreateBody(Wall);
		InContext.Expect(Reused == Target, "a destroyed slot is reused");
	});

	InSuite.Add("Physics world: sweep-all and radius queries list the accepted bodies", [](TestContext& InContext)
	{
		model::PhysicsWorld World;
		model::BodyDef Definition;
		Definition.Type = model::BodyType::Kinematic;
		Definition.BodyShape.Kind = model::ShapeKind::Box;
		Definition.BodyShape.Size = model::Vec2f(10.0f, 10.0f);
		Definition.BodyShape.Anchor = model::Vec2f(0.0f, 0.0f);
		Definition.Position = model::Vec2f(100.0f, 0.0f);
		const model::BodyId A = World.CreateBody(Definition);
		Definition.Position = model::Vec2f(200.0f, 0.0f);
		const model::BodyId B = World.CreateBody(Definition);
		TArray<model::BodyId> Found;
		World.SweepAll(model::Vec2f(0.0f, 5.0f), model::Vec2f(300.0f, 5.0f), model::Vec2f(1.0f, 1.0f), [](model::BodyId) { return true; }, Found);
		InContext.ExpectEqual(static_cast<int>(Found.Size()), 2, "both bodies on the path are listed");
		Found.Clear();
		World.SweepAll(model::Vec2f(0.0f, 5.0f), model::Vec2f(300.0f, 5.0f), model::Vec2f(1.0f, 1.0f), [B](model::BodyId InBody) { return InBody == B; }, Found);
		InContext.Expect(Found.Size() == 1 && Found[0] == B, "the filter narrows the list");
		Found.Clear();
		World.QueryRadius(model::Vec2f(105.0f, 5.0f), 20.0f, [](model::BodyId) { return true; }, Found);
		InContext.Expect(Found.Size() == 1 && Found[0] == A, "only the body inside the blast radius is listed");
	});

	InSuite.Add("Physics events: a chip reports the broken cells and heading, the queue replays in order", [](TestContext& InContext)
	{
		model::GridBody Body(4, 1);
		const model::RectF Box(0.0f, 0.0f, 64.0f, 64.0f);
		model::GridCarveResult Result;
		Body.Chip(Box, model::RectF(0.0f, 0.0f, 16.0f, 64.0f), model::HitSide::Left, 1, 1, Result);
		InContext.ExpectEqual(Result.Removed, 4, "one column of four cells fell");
		InContext.ExpectEqual(static_cast<int>(Result.BrokenCells.Size()), 4, "all four are listed");
		InContext.ExpectNear(Result.Heading.X, 1.0f, 0.001f, "a hit on the left side travels right");

		struct Recorder final : model::IPhysicsEventSink
		{
			int Order = 0;
			int CarveAt = -1;
			int ContactAt = -1;
			void OnCarved(const model::CarveEvent&) override { CarveAt = Order++; }
			void OnContact(const model::ContactEvent&) override { ContactAt = Order++; }
		};
		model::PhysicsEventQueue Queue;
		Queue.OnContact(model::ContactEvent());
		Queue.OnCarved(model::CarveEvent());
		Recorder Sink;
		Queue.Drain(Sink);
		InContext.Expect(Sink.ContactAt == 0 && Sink.CarveAt == 1, "events come back in the order they were queued");
		InContext.Expect(Queue.IsEmpty(), "the queue is empty after a drain");
	});

	InSuite.Add("PhysicsWorld: a dynamic body stops at a static wall and reports a hit with a normal", [](TestContext& InContext)
	{
		model::PhysicsWorld World;
		model::BodyDef Wall;
		Wall.Type = model::BodyType::Static;
		Wall.BodyShape.Size = { 64.0f, 64.0f };
		Wall.BodyShape.Anchor = { 0.0f, 0.0f };
		Wall.Position = { 100.0f, 0.0f };
		Wall.UserTag = 7;
		World.CreateBody(Wall);

		model::BodyDef Shell;
		Shell.Type = model::BodyType::Dynamic;
		Shell.BodyShape.Kind = model::ShapeKind::Circle;
		Shell.BodyShape.Size = { 8.0f, 8.0f };
		Shell.Profile.ObjectType = model::CollisionChannel::Projectile;
		Shell.Position = { 0.0f, 30.0f };
		Shell.UserTag = 3;
		const model::BodyId ShellId = World.CreateBody(Shell);
		World.SetVelocity(ShellId, { 200.0f, 0.0f });
		World.Step(1.0f);

		InContext.ExpectNear(World.GetPosition(ShellId).X, 96.0f, 0.01f, "it stops with its edge on the wall");
		struct Recorder final : model::IPhysicsEventSink
		{
			int Hits = 0;
			model::ContactEvent Last;
			void OnContact(const model::ContactEvent& InEvent) override { ++Hits; Last = InEvent; }
			void OnCarved(const model::CarveEvent&) override {}
		};
		Recorder Sink;
		World.DrainEvents(Sink);
		InContext.ExpectEqual(Sink.Hits, 1, "one hit event");
		InContext.ExpectEqual(static_cast<int>(Sink.Last.OtherTag), 7, "the wall tag comes back");
		InContext.ExpectNear(Sink.Last.Hit.Normal.X, -1.0f, 0.001f, "the wall's surface faces back");
		InContext.ExpectNear(Sink.Last.Hit.ImpactPoint.X, 100.0f, 0.01f, "the impact is on the wall face");
	});

	InSuite.Add("PhysicsWorld: collision responses decide what blocks, and a carved grid lets a body through", [](TestContext& InContext)
	{
		model::PhysicsWorld World;
		model::BodyDef Wall;
		Wall.BodyShape.Kind = model::ShapeKind::Grid;
		Wall.BodyShape.GridCellsPerSide = 4;
		Wall.BodyShape.Size = { 64.0f, 64.0f };
		Wall.BodyShape.Anchor = { 0.0f, 0.0f };
		Wall.Position = { 100.0f, 0.0f };
		const model::BodyId WallId = World.CreateBody(Wall);

		model::CollisionProfile Ghost;
		Ghost.ObjectType = model::CollisionChannel::Projectile;
		Ghost.SetResponseTo(model::CollisionChannel::WorldStatic, model::CollisionResponse::Ignore);
		model::Shape Dot;
		Dot.Size = { 4.0f, 4.0f };
		model::HitResult Hit;
		InContext.Expect(!World.SweepSingle({ 0.0f, 30.0f }, { 200.0f, 30.0f }, Dot, Ghost, Hit), "a body that ignores the static channel passes through");
		model::CollisionProfile Solid;
		Solid.ObjectType = model::CollisionChannel::Projectile;
		InContext.Expect(World.SweepSingle({ 0.0f, 30.0f }, { 200.0f, 30.0f }, Dot, Solid, Hit), "a blocking one is stopped");

		const model::GridCarveResult Carved = World.Carve(WallId, model::RectF(100.0f, 0.0f, 16.0f, 64.0f), model::HitSide::Left, 16, 1);
		InContext.Expect(Carved.Removed > 0, "the carve removed cells");
		InContext.Expect(!World.SweepSingle({ 0.0f, 30.0f }, { 200.0f, 30.0f }, Dot, Solid, Hit), "a fully carved wall no longer blocks");
	});

	InSuite.Add("GridBody: blocks only where cells stand and chips from the hit side", [](TestContext& InContext)
	{
		model::GridBody Body(4, 1);
		const model::RectF Box(0.0f, 0.0f, 64.0f, 64.0f);
		InContext.Expect(Body.Blocks(Box, model::RectF(60.0f, 30.0f, 8.0f, 8.0f)), "a full wall blocks at its edge");
		const int Removed = Body.Chip(Box, model::RectF(48.0f, 0.0f, 16.0f, 64.0f), model::HitSide::Right, 1, 1);
		InContext.Expect(Removed > 0, "a hit from the right chips cells");
		InContext.Expect(!Body.Blocks(Box, model::RectF(60.0f, 30.0f, 8.0f, 8.0f)), "the chipped edge no longer blocks");
		InContext.Expect(Body.Blocks(Box, model::RectF(2.0f, 30.0f, 8.0f, 8.0f)), "the far side still blocks");
		InContext.Expect(Body.Chip(Box, model::RectF(200.0f, 0.0f, 10.0f, 10.0f), model::HitSide::Right, 1, 1) == 0, "a hit away from the box chips nothing");
	});
}

} // namespace tests
} // namespace game

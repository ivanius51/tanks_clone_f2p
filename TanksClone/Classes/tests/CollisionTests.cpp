#include "tests/CollisionTests.h"

#include "model/Collision.h"
#include "model/RenderLayer.h"
#include "model/WallBody.h"

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

	InSuite.Add("WallBody: blocks only where cells stand and chips from the hit side", [](TestContext& InContext)
	{
		model::WallBody Body(4, 1);
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

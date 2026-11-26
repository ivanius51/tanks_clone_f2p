#include "tests/TileMergeTests.h"

#include <cmath>

#include "model/TileMergeField.h"

namespace game
{
namespace tests
{

namespace
{
	constexpr float Sample = 0.02f;

	// Width in cells of the shape along a horizontal line through the middle of the row.
	float MeasureWidth(const model::TileMergeField& InField, const model::TileMergeSettings& InSettings, const float InY)
	{
		int Inside = 0;
		const int Steps = static_cast<int>(static_cast<float>(InField.GetColumns()) / Sample);
		for ( int Step = 0; Step < Steps; ++Step )
		{
			if ( InField.Sample(static_cast<float>(Step) * Sample, InY, 0.0f, 0.0f, InSettings).bInside )
			{
				++Inside;
			}
		}
		return static_cast<float>(Inside) * Sample;
	}

	model::TileMergeField MakeField(const int InColumns, const int InRows)
	{
		model::TileMergeField Field;
		Field.Resize(InColumns, InRows);
		return Field;
	}
}

void AddTileMergeTests(TestSuite& InSuite)
{
	InSuite.Add("TileMerge: an isolated tile is about the configured share of its cell", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		Settings.SetSize(0.8f);
		model::TileMergeField Field = MakeField(5, 5);
		Field.SetCellInstant(2, 2, model::MergeMaterial::Water);
		const float Width = MeasureWidth(Field, Settings, 2.5f);
		InContext.ExpectNear(Width, 0.8f, 0.1f, "the width in the middle row is the size setting");
	});

	InSuite.Add("TileMerge: neighbours of one type merge, a flat edge stays straight", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		Settings.SetSize(0.8f);
		model::TileMergeField Field = MakeField(6, 5);
		for ( int Column = 1; Column <= 4; ++Column )
		{
			Field.SetCellInstant(Column, 2, model::MergeMaterial::Brick);
		}
		const float Width = MeasureWidth(Field, Settings, 2.5f);
		InContext.ExpectNear(Width, 3.8f, 0.15f, "four cells in a row make one bar, no gaps between them");
		bool bFlatTop = true;
		const float Middle = Field.Sample(2.0f, 2.1f, 0.0f, 0.0f, Settings).bInside ? 1.0f : 0.0f;
		const float Next = Field.Sample(3.0f, 2.1f, 0.0f, 0.0f, Settings).bInside ? 1.0f : 0.0f;
		bFlatTop = Middle == Next;
		InContext.Expect(bFlatTop, "the top edge is at the same height over every cell of the bar");
	});

	InSuite.Add("TileMerge: a square-cornered material keeps outer corners squarer than a round one", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		Settings.SetSize(0.8f);
		model::TileMergeField Field = MakeField(3, 3);
		Field.SetCellInstant(1, 1, model::MergeMaterial::Brick);
		const bool bBrickCorner = Field.Sample(1.12f, 1.12f, 0.0f, 0.0f, Settings).bInside;
		Field.SetCellInstant(1, 1, model::MergeMaterial::Water);
		const bool bWaterCorner = Field.Sample(1.12f, 1.12f, 0.0f, 0.0f, Settings).bInside;
		InContext.Expect(bBrickCorner, "a brick corner point near the cell corner is filled");
		InContext.Expect(!bWaterCorner, "the same point of a water tile is cut off by the rounding");
	});

	InSuite.Add("TileMerge: two touching types share one border without a gap", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		model::TileMergeField Field = MakeField(4, 3);
		Field.SetCellInstant(1, 1, model::MergeMaterial::Brick);
		Field.SetCellInstant(2, 1, model::MergeMaterial::Stone);
		const model::MergeSample Left = Field.Sample(1.9f, 1.5f, 0.0f, 0.0f, Settings);
		const model::MergeSample Right = Field.Sample(2.1f, 1.5f, 0.0f, 0.0f, Settings);
		InContext.Expect(Left.bInside && Right.bInside, "both sides of the border are filled");
		InContext.Expect(Left.Material == model::MergeMaterial::Brick && Right.Material == model::MergeMaterial::Stone, "each side keeps its own material");
	});

	InSuite.Add("TileMerge: a removed cell fades out over the fade time and lists the cells that moved", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		model::TileMergeField Field = MakeField(3, 3);
		Field.SetCellInstant(1, 1, model::MergeMaterial::Brick);
		Field.ClearCell(1, 1);
		InContext.Expect(!Field.IsOccupied(1, 1), "the cell is free at once");
		InContext.Expect(Field.Step(0.05f, 0.25f), "still moving after a short step");
		InContext.ExpectEqual(static_cast<int>(Field.GetChangedCells().Size()), 1, "only the removed cell changed");
		int Guard = 0;
		while ( Field.Step(0.05f, 0.25f) && Guard < 100 )
		{
			++Guard;
		}
		InContext.Expect(Guard < 100, "it settles");
		InContext.Expect(!Field.Sample(1.5f, 1.5f, 0.0f, 0.0f, Settings).bInside, "nothing is left of the tile");
	});

	InSuite.Add("TileMerge: settings are clamped to their valid range", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		Settings.SetSize(5.0f);
		InContext.ExpectNear(Settings.GetSize(), 1.0f, 0.0001f, "size tops out at the full cell");
		Settings.SetSize(0.1f);
		InContext.ExpectNear(Settings.GetSize(), 0.6f, 0.0001f, "size has a floor");
		Settings.SetOverlayAmount(2.0f);
		InContext.ExpectNear(Settings.GetOverlayAmount(), 0.6f, 0.0001f, "overlay has a ceiling");
	});
}

} // namespace tests
} // namespace game

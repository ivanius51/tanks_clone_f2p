#include "tests/TileMergeTests.h"

#include <cmath>

#include "model/TileMergeBaker.h"
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
	InSuite.Add("GraphicsQuality: the benchmark picks the highest level that fits the budget", [](TestContext& InContext)
	{
		model::GraphicsMeasure Slow;
		Slow.CpuRegionMs32 = 40.0f;
		InContext.Expect(model::PickGraphicsQuality(Slow, 10.0f) == model::GraphicsQuality::Low, "a slow CPU without a GPU gets the old walls");
		model::GraphicsMeasure Middle;
		Middle.CpuRegionMs32 = 12.0f;
		InContext.Expect(model::PickGraphicsQuality(Middle, 10.0f) == model::GraphicsQuality::Middle, "a quarter of the cost fits");
		model::GraphicsMeasure Fast;
		Fast.CpuRegionMs32 = 5.0f;
		InContext.Expect(model::PickGraphicsQuality(Fast, 10.0f) == model::GraphicsQuality::High, "32 pixels fit, 96 do not");
		model::GraphicsMeasure Gpu;
		Gpu.CpuRegionMs32 = 40.0f;
		Gpu.bGpuWorks = true;
		Gpu.GpuFrameMs = 2.0f;
		InContext.Expect(model::PickGraphicsQuality(Gpu, 10.0f) == model::GraphicsQuality::Ultra, "a fast GPU shader gets Ultra");
		Gpu.GpuFrameMs = 30.0f;
		InContext.Expect(model::PickGraphicsQuality(Gpu, 10.0f) == model::GraphicsQuality::High, "a slow GPU frame stays at High");
	});

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

	InSuite.Add("TileMerge baker: pixels repeat for a seed, chunks join seamlessly, empty cells stay transparent", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		model::TileMergeField Field = MakeField(4, 2);
		Field.SetCellInstant(0, 0, model::MergeMaterial::Brick);
		Field.SetCellInstant(1, 0, model::MergeMaterial::Stone);
		Field.SetCellInstant(2, 0, model::MergeMaterial::Wood);
		Field.SetCellInstant(1, 1, model::MergeMaterial::Water);
		const model::TileMergeBaker Baker(Settings, 5);
		const model::MergeChunk Whole{ 0, 0, 4, 2, 16 };
		TArray<std::uint8_t> First(Whole.GetByteCount());
		TArray<std::uint8_t> Again(Whole.GetByteCount());
		Baker.Bake(Field, Whole, First.GetData());
		Baker.Bake(Field, Whole, Again.GetData());
		InContext.Expect(First == Again, "the same seed gives the same pixels");
		const model::MergeChunk Right{ 2, 0, 2, 2, 16 };
		TArray<std::uint8_t> Part(Right.GetByteCount());
		Baker.Bake(Field, Right, Part.GetData());
		bool bSame = true;
		for ( int Y = 0; Y < Right.GetHeight() && bSame; ++Y )
		{
			for ( int X = 0; X < Right.GetWidth() && bSame; ++X )
			{
				const size_t Whole_ = (static_cast<size_t>(Y) * static_cast<size_t>(Whole.GetWidth()) + static_cast<size_t>(X + 32)) * 4;
				const size_t Part_ = (static_cast<size_t>(Y) * static_cast<size_t>(Right.GetWidth()) + static_cast<size_t>(X)) * 4;
				bSame = First[Whole_] == Part[Part_] && First[Whole_ + 3] == Part[Part_ + 3];
			}
		}
		InContext.Expect(bSame, "a chunk baked alone matches the same area of the whole map");
		const size_t Empty = (static_cast<size_t>(24) * static_cast<size_t>(Whole.GetWidth()) + 56) * 4;
		InContext.ExpectEqual(static_cast<int>(First[Empty + 3]), 0, "a cell with no tile is transparent");
		const size_t Solid = (static_cast<size_t>(8) * static_cast<size_t>(Whole.GetWidth()) + 8) * 4;
		InContext.ExpectEqual(static_cast<int>(First[Solid + 3]), 255, "the middle of a tile is opaque");
	});

	InSuite.Add("TileMerge baker: the overlay covers a fifth to two fifths of a tile at the default amount", [](TestContext& InContext)
	{
		model::TileMergeSettings Settings;
		model::TileMergeSettings Plain;
		Plain.SetOverlayAmount(0.0f);
		model::TileMergeField Field = MakeField(6, 6);
		for ( int Row = 1; Row <= 4; ++Row )
		{
			for ( int Column = 1; Column <= 4; ++Column )
			{
				Field.SetCellInstant(Column, Row, model::MergeMaterial::Stone);
			}
		}
		const model::MergeChunk Chunk{ 0, 0, 6, 6, 16 };
		TArray<std::uint8_t> With(Chunk.GetByteCount());
		TArray<std::uint8_t> Without(Chunk.GetByteCount());
		model::TileMergeBaker(Settings, 3).Bake(Field, Chunk, With.GetData());
		model::TileMergeBaker(Plain, 3).Bake(Field, Chunk, Without.GetData());
		int Opaque = 0;
		int Changed = 0;
		for ( size_t Index = 0; Index < With.Size(); Index += 4 )
		{
			if ( With[Index + 3] != 255 )
			{
				continue;
			}
			++Opaque;
			Changed += With[Index] != Without[Index] ? 1 : 0;
		}
		const float Share = static_cast<float>(Changed) / static_cast<float>(Opaque);
		InContext.Expect(Share >= 0.2f && Share <= 0.4f, "the speckled share of the surface is 20-40%");
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
		Settings.SetSizeRange(1.2f, 0.7f);
		const float Resolved = Settings.ResolveSize(7u);
		InContext.Expect(Resolved >= 0.7f - 0.0001f && Resolved <= 1.0f + 0.0001f, "size range is ordered and clamped");
	});
}

} // namespace tests
} // namespace game

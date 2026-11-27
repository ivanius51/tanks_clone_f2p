#include "tests/TileMergeBenchmark.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "model/Log.h"
#include "model/TileMergeBaker.h"
#include "model/TileMergeBakerCached.h"
#include "model/TileMergeField.h"
#include "templates/TContainers.h"

namespace game
{
namespace tests
{

namespace
{
	constexpr int FieldColumns = 32;
	constexpr int FieldRows = 32;
	constexpr int Subdivision = 2;
	constexpr int ChunkCells = 8;
	constexpr int RegionCells = 5;
	constexpr int Repeats = 5;
	constexpr int PixelSizes[] = { 16, 32, 64 };

	template <typename TWork>
	double MeasureMs(TWork&& InWork)
	{
		const auto Start = std::chrono::steady_clock::now();
		for ( int Index = 0; Index < Repeats; ++Index )
		{
			InWork();
		}
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count() / Repeats;
	}

	model::TileMergeField MakeWalls()
	{
		model::TileMergeField Field;
		Field.Resize(FieldColumns, FieldRows);
		for ( int Row = 0; Row < FieldRows; ++Row )
		{
			for ( int Column = 0; Column < FieldColumns; ++Column )
			{
				Field.SetCellInstant(Column, Row, model::MergeMaterial::Brick);
			}
		}
		return Field;
	}
}

std::string RunTileMergeBenchmark()
{
	const model::TileMergeSettings Settings;
	const model::TileMergeField Field = MakeWalls();
	std::string Report = "Wall merge benchmark (ms per call, average of " + std::to_string(Repeats) + "; chunk " + std::to_string(ChunkCells) + "x" + std::to_string(ChunkCells) + " field cells, subdivision " + std::to_string(Subdivision) + ")\n";
	Report += "  cell_pixels | baker      | chunk color | chunk shadow | region color | region shadow | speed-up | max pixel diff\n";
	for ( const int CellPixels : PixelSizes )
	{
		const int FieldCellPixels = CellPixels / Subdivision;
		const model::MergeChunk Chunk{ 8, 8, ChunkCells, ChunkCells, FieldCellPixels, 1 };
		const model::MergeChunk ShadowChunk{ 8, 8, ChunkCells, ChunkCells, FieldCellPixels, 0 };
		const model::MergeChunk Region{ 10, 10, RegionCells, RegionCells, FieldCellPixels, 1 };
		const model::MergeChunk ShadowRegion{ 10, 10, RegionCells, RegionCells, FieldCellPixels, 0 };
		const model::TileMergeBaker Reference(Settings, 1, Subdivision);
		const model::TileMergeBakerCached Cached(Settings, 1, Subdivision);
		const model::IMergeBaker* const Bakers[] = { &Reference, &Cached };
		TArray<std::uint8_t> Pixels(Chunk.GetByteCount());
		TArray<std::uint8_t> ReferencePixels(Chunk.GetByteCount());
		Reference.Bake(Field, Chunk, ReferencePixels.GetData());
		double ReferenceTotal = 1.0;
		for ( const model::IMergeBaker* const Baker : Bakers )
		{
			const double ChunkColor = MeasureMs([&]() { Baker->Bake(Field, Chunk, Pixels.GetData()); });
			const double ChunkShadow = MeasureMs([&]() { Baker->BakeShadow(Field, ShadowChunk, Pixels.GetData()); });
			const double RegionColor = MeasureMs([&]() { Baker->Bake(Field, Region, Pixels.GetData()); });
			const double RegionShadow = MeasureMs([&]() { Baker->BakeShadow(Field, ShadowRegion, Pixels.GetData()); });
			const double Total = ChunkColor + ChunkShadow;
			ReferenceTotal = Baker == &Reference ? Total : ReferenceTotal;
			Baker->Bake(Field, Chunk, Pixels.GetData());
			int MaxDiff = 0;
			for ( size_t Index = 0; Index < Pixels.Size(); ++Index )
			{
				MaxDiff = std::max(MaxDiff, std::abs(static_cast<int>(Pixels[Index]) - static_cast<int>(ReferencePixels[Index])));
			}
			char Line[256];
			std::snprintf(Line, sizeof(Line), "  %11d | %-10s | %11.2f | %12.2f | %12.2f | %13.2f | %7.1fx | %d\n", CellPixels, Baker->GetName(), ChunkColor, ChunkShadow, RegionColor, RegionShadow, ReferenceTotal / Total, MaxDiff);
			Report += Line;
		}
	}
	model::Log::Info("%s", Report.c_str());
	return Report;
}

} // namespace tests
} // namespace game

#include "ui/GraphicsBenchmark.h"

#include <algorithm>
#include <chrono>

#include "model/Log.h"
#include "model/TileMergeBaker.h"
#include "model/TileMergeBakerCached.h"
#include "model/TileMergeField.h"
#include "templates/TContainers.h"
#include "ui/TileMergeGpu.h"

namespace game::view::cocos
{
namespace
{
constexpr int FieldSize = 32;
constexpr int MeasureSubdivision = 2;
constexpr int RegionCells = 5;
constexpr int RegionFirst = 10;
constexpr int MeasureCellPixels = 32;
constexpr int Repeats = 3;

float MeasureCpuRegionMs(const model::TileMergeSettings& InSettings)
{
	model::TileMergeField Field;
	Field.Resize(FieldSize, FieldSize);
	for ( int Row = 0; Row < FieldSize; ++Row )
	{
		for ( int Column = 0; Column < FieldSize; ++Column )
		{
			Field.SetCellInstant(Column, Row, model::MergeMaterial::Brick);
		}
	}
	const int FieldCellPixels = MeasureCellPixels / MeasureSubdivision;
	const model::MergeChunk Color{ RegionFirst, RegionFirst, RegionCells, RegionCells, FieldCellPixels, 1 };
	const model::MergeChunk Shadow{ RegionFirst, RegionFirst, RegionCells, RegionCells, FieldCellPixels, 0 };
	const model::TileMergeBakerCached Baker(InSettings, 1, MeasureSubdivision);
	TArray<std::uint8_t> Pixels(std::max(Color.GetByteCount(), Shadow.GetByteCount()));
	const auto Start = std::chrono::steady_clock::now();
	for ( int Index = 0; Index < Repeats; ++Index )
	{
		Baker.Bake(Field, Color, Pixels.GetData());
		Baker.BakeShadow(Field, Shadow, Pixels.GetData());
	}
	return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - Start).count() / static_cast<float>(Repeats);
}
}

model::GraphicsMeasure MeasureGraphics(const model::TileMergeSettings& InSettings)
{
	model::GraphicsMeasure Result;
	Result.CpuRegionMs32 = MeasureCpuRegionMs(InSettings);
	const float GpuMs = TileMergeGpu::MeasureFrameMs(InSettings);
	Result.bGpuWorks = GpuMs >= 0.0f;
	Result.GpuFrameMs = std::max(GpuMs, 0.0f);
	model::Log::Info("Graphics benchmark: cpu region %.2f ms, gpu %s %.2f ms", Result.CpuRegionMs32, Result.bGpuWorks ? "ok" : "unavailable", Result.GpuFrameMs);
	return Result;
}
} // namespace game::view::cocos

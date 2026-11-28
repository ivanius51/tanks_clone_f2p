#pragma once

#include <string_view>

namespace game::model
{
/** Picture quality levels; the wall merge settings of each level are in configs/balance/tile_merge.json ("quality"). */
enum class GraphicsQuality
{
	Low,
	Middle,
	High,
	Ultra
};

inline constexpr int GraphicsQualityCount = 4;

/** What the first-run benchmark measured on this device. */
struct GraphicsMeasure
{
	bool bGpuWorks = false;
	float GpuFrameMs = 0.0f;
	/** Cached CPU baker, color plus shadow of one wall region at 32 pixels per cell. */
	float CpuRegionMs32 = 0.0f;
};

std::string_view GetGraphicsQualityKey(GraphicsQuality InQuality);

/** The highest level whose estimated cost fits the budget: the GPU shader or a CPU bake under the budget allows High and Ultra. */
GraphicsQuality PickGraphicsQuality(const GraphicsMeasure& InMeasure, float InBudgetMs);
} // namespace game::model

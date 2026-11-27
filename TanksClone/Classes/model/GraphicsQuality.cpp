#include "model/GraphicsQuality.h"

namespace game::model
{
namespace
{
constexpr std::string_view Keys[GraphicsQualityCount]{ "very_low", "low", "middle", "high", "ultra" };
constexpr float MiddleCostScale = 0.25f;
constexpr float UltraCostScale = 9.0f;
}

std::string_view GetGraphicsQualityKey(const GraphicsQuality InQuality)
{
	return Keys[static_cast<int>(InQuality)];
}

GraphicsQuality PickGraphicsQuality(const GraphicsMeasure& InMeasure, const float InBudgetMs)
{
	const bool bGpuFits = InMeasure.bGpuWorks && InMeasure.GpuFrameMs < InBudgetMs;
	if ( bGpuFits || InMeasure.CpuRegionMs32 * UltraCostScale < InBudgetMs )
	{
		return GraphicsQuality::Ultra;
	}
	if ( InMeasure.bGpuWorks || InMeasure.CpuRegionMs32 < InBudgetMs )
	{
		return GraphicsQuality::High;
	}
	if ( InMeasure.CpuRegionMs32 * MiddleCostScale < InBudgetMs )
	{
		return GraphicsQuality::Middle;
	}
	return GraphicsQuality::Low;
}
} // namespace game::model

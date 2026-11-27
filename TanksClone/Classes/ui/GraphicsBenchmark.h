#pragma once

#include "model/GraphicsQuality.h"
#include "model/TileMergeSettings.h"

namespace game::view::cocos
{
/**
 * The first-run benchmark behind the graphics quality: times the cached CPU baker on one wall region and a full-map
 * frame of the GPU shader (a GL context is needed), then picks the highest quality that fits the budget.
 */
model::GraphicsMeasure MeasureGraphics(const model::TileMergeSettings& InSettings);
} // namespace game::view::cocos

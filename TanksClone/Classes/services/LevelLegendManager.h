#pragma once

#include "model/LevelLegend.h"

namespace tanks
{

/** Loads the level legend (configs/level_legend.json) at startup and hands it to the level loader, the scenes and the editor. */
class LevelLegendManager
{
public:
	void Init();

	const model::LevelLegend& Get() const { return Legend; }

private:
	model::LevelLegend Legend;
};

} // namespace tanks

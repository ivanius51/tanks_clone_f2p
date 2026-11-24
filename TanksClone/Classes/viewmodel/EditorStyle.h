#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/EditorLayout.h"
#include "model/MathTypes.h"

namespace tanks::viewmodel
{
/**
 * Look and limits of the level editor (configs/ui/editor.json): dock layout, colors, the enemy counter range.
 * The initial values are the fallbacks for missing keys.
 */
struct EditorStyle
{
	model::EditorLayoutSettings Layout;
	float IconFill = 0.8f;
	float EnemyLabelFontSize = 22.0f;
	float MessageBottom = 40.0f;
	float ButtonHeight = 36.0f;
	int MinEnemies = 1;
	int MaxEnemies = 40;
	int MaxAlive = 4;
	int LibraryRows = 8;
	float MinZoom = 0.1f;
	float MaxZoom = 2.0f;
	float ZoomStep = 1.25f;
	std::string DevResourcesRoot;
	model::Color PanelColor{ 28, 31, 41 };
	model::Color PanelBorder{ 89, 102, 128 };
	model::Color EraseColor{ 230, 89, 77 };

	static EditorStyle FromConfig(const model::IConfigNode* InNode, std::vector<std::string>& OutMissing);
};
} // namespace tanks::viewmodel

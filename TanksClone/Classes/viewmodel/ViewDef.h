#pragma once

#include <string>
#include <utility>

#include "model/ConfigNode.h"
#include "templates/TContainers.h"

namespace game::viewmodel
{
/** Sample widgets a view adds into one of its containers, so the editor shows the window as the game fills it. */
struct ViewItemDef
{
	std::string Path;
	std::string Widget;
	TArray<std::string> Texts;
};

/**
 * A view (configs/ui/views/<name>.json): a window the widget editor can open, which root widget it is and the sample
 * texts and items the game would set from code. The game does not read these files, only the editor does.
 */
struct ViewDef
{
	std::string Id;
	std::string Title;
	std::string Root;
	std::string Menu;
	TArray<std::pair<std::string, std::string>> Texts;
	TArray<ViewItemDef> Items;

	static ViewDef FromConfig(const model::IConfigNode& InNode, const std::string& InId);
};
} // namespace game::viewmodel

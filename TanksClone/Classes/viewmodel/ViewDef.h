#pragma once

#include <string>
#include <utility>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::viewmodel
{

/** Sample widgets a view adds into one of its containers, so the editor shows the window as the game fills it. */
struct ViewItemDef
{
	// The container: child ids joined by '/', empty for the root widget.
	std::string Path;
	std::string Widget;
	std::vector<std::string> Texts;
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
	// A menu file (configs/ui/menus): its root widget, texts and buttons replace Root, Texts and Items.
	std::string Menu;
	// Child path ('/' between nested widgets) and the text to show in it.
	std::vector<std::pair<std::string, std::string>> Texts;
	std::vector<ViewItemDef> Items;

	static ViewDef FromConfig(const model::IConfigNode& InNode, const std::string& InId);
};

} // namespace tanks::viewmodel

#pragma once

#include <string_view>

#include "ui/LayoutWidget.h"

namespace tanks::view::cocos
{

/** Creates the widgets described by the widget files (configs/ui/widgets/<name>.json, registered as cfg.widget.<name>). */
class WidgetFactory
{
public:
	// nullptr (and logged) when no widget file has this id.
	static LayoutWidget* Create(std::string_view InId);

	// A keycap frame around a key name ("W", "Ctrl"); a long name shrinks to fit.
	static LayoutWidget* CreateKeycap(const std::string& InKeyName);
	// A sprite frame in the box the widget file gives; nullptr when the frame is not loaded.
	static LayoutWidget* CreateIcon(const std::string& InFrame);
};

} // namespace tanks::view::cocos

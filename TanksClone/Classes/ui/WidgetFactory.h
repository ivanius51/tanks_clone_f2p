#pragma once

#include <string_view>

#include "ui/LayoutWidget.h"

namespace game::view::cocos
{
/** Creates the widgets described by the widget files (configs/ui/widgets/<name>.json, registered as cfg.widget.<name>). */
class WidgetFactory
{
public:
	static LayoutWidget* Create(std::string_view InId);

	static LayoutWidget* CreateKeycap(const std::string& InKeyName);
	static LayoutWidget* CreateIcon(const std::string& InFrame);
};
} // namespace game::view::cocos

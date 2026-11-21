#pragma once

#include "viewmodel/EditorStyle.h"
#include "viewmodel/HudLayout.h"
#include "viewmodel/MenuLayout.h"
#include "viewmodel/WidgetStyle.h"

namespace tanks
{

/**
 * The UI layout and look read from configs/ui/*.json at startup (before the loading screen shows), handed to the
 * screens: widgets, menu screens, HUD, editor. A missing key keeps its default and is logged once.
 */
class UiStyle
{
public:
	UiStyle();
	UiStyle(const UiStyle&) = delete;
	UiStyle& operator=(const UiStyle&) = delete;

	const viewmodel::WidgetStyle& GetWidgets() const;
	const viewmodel::MenuLayout& GetMenu() const;
	const viewmodel::HudLayout& GetHud() const;
	const viewmodel::EditorStyle& GetEditor() const;

private:
	viewmodel::WidgetStyle Widgets;
	viewmodel::MenuLayout Menu;
	viewmodel::HudLayout Hud;
	viewmodel::EditorStyle Editor;
};

} // namespace tanks

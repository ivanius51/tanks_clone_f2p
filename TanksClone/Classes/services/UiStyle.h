#pragma once

#include <map>
#include <string>
#include <string_view>

#include "viewmodel/EditorStyle.h"
#include "viewmodel/HudLayout.h"
#include "viewmodel/MenuLayout.h"
#include "viewmodel/SplashStyle.h"
#include "viewmodel/WidgetDef.h"
#include "viewmodel/WidgetStyle.h"

namespace tanks
{

class ConfigReader;

/**
 * The UI layout and look read from the configs/ui JSON files at startup (before the loading screen shows), handed to the
 * screens: widgets, menu screens, HUD, editor, startup splash. A missing key keeps its default and is logged once.
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
	const viewmodel::SplashStyle& GetSplash() const;
	// The widget file configs/ui/widgets/<Id>.json registered as "cfg.widget.<Id>"; nullptr (and logged) when unknown.
	const viewmodel::WidgetDef* FindWidget(std::string_view InId) const;
	// Widgets read with this variant's overrides ("portrait", "compact"; empty = the base layout) from now on; call ReloadWidgets to apply it.
	void SetWidgetVariant(std::string_view InVariant);
	// Reads the widget files again from disk (debug and editor use); widgets already on screen need LayoutWidget::ReloadAll.
	void ReloadWidgets();

private:
	void LoadWidgets(const ConfigReader& InReader);

	viewmodel::WidgetStyle Widgets;
	viewmodel::MenuLayout Menu;
	viewmodel::HudLayout Hud;
	viewmodel::EditorStyle Editor;
	viewmodel::SplashStyle Splash;
	std::map<std::string, viewmodel::WidgetDef, std::less<>> WidgetDefs;
	std::string WidgetVariant;
};

} // namespace tanks

#pragma once

#include <map>
#include <string>
#include <string_view>

#include "model/Animation.h"
#include "viewmodel/EditorStyle.h"
#include "viewmodel/HudLayout.h"
#include "viewmodel/MenuLayout.h"
#include "viewmodel/SplashStyle.h"
#include "viewmodel/MenuDef.h"
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
	// The animation file configs/ui/animations/<Id>.json registered as "cfg.anim.<Id>"; nullptr (and logged) when unknown.
	const model::AnimationDef* FindAnimation(std::string_view InId) const;
	// The menu file configs/ui/menus/<Id>.json registered as "cfg.menu.<Id>"; nullptr (and logged) when unknown.
	const viewmodel::MenuDef* FindMenu(std::string_view InId) const;
	const std::map<std::string, viewmodel::MenuDef, std::less<>>& GetMenus() const;
	// Puts an edited menu in place of the loaded one with the same id (the widget editor, after saving).
	void ReplaceMenu(const viewmodel::MenuDef& InDef);
	void SetWidgetVariant(std::string_view InVariant);
	// Reads the widget files again from disk (debug and editor use); widgets already on screen need LayoutWidget::ReloadAll.
	void ReloadWidgets();
	// Puts an edited widget in place of the loaded one with the same id (the widget editor); widgets on screen need LayoutWidget::ReloadAll.
	void ReplaceWidget(const viewmodel::WidgetDef& InDef);

private:
	void LoadWidgets(const ConfigReader& InReader);
	void LoadAnimations(const ConfigReader& InReader);
	void LoadMenus(const ConfigReader& InReader);

	viewmodel::WidgetStyle Widgets;
	viewmodel::MenuLayout Menu;
	viewmodel::HudLayout Hud;
	viewmodel::EditorStyle Editor;
	viewmodel::SplashStyle Splash;
	std::map<std::string, viewmodel::WidgetDef, std::less<>> WidgetDefs;
	std::map<std::string, model::AnimationDef, std::less<>> AnimationDefs;
	std::map<std::string, viewmodel::MenuDef, std::less<>> MenuDefs;
	std::string WidgetVariant;
};

} // namespace tanks

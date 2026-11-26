#pragma once

#include <map>
#include <string>
#include <string_view>

#include "model/Animation.h"
#include "templates/TContainers.h"
#include "viewmodel/EditorStyle.h"
#include "viewmodel/HudLayout.h"
#include "viewmodel/MenuLayout.h"
#include "viewmodel/SplashStyle.h"
#include "viewmodel/MenuDef.h"
#include "viewmodel/WidgetDef.h"
#include "viewmodel/WidgetStyle.h"

namespace game
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
	const viewmodel::WidgetDef* FindWidget(std::string_view InId) const;
	const model::AnimationDef* FindAnimation(std::string_view InId) const;
	const viewmodel::MenuDef* FindMenu(std::string_view InId) const;
	const TMap<FPersistentString, viewmodel::MenuDef>& GetMenus() const;
	void ReplaceMenu(const viewmodel::MenuDef& InDef);
	void SetWidgetVariant(std::string_view InVariant);
	void ReloadWidgets();
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
	TMap<FPersistentString, viewmodel::WidgetDef> WidgetDefs;
	TMap<FPersistentString, model::AnimationDef> AnimationDefs;
	TMap<FPersistentString, viewmodel::MenuDef> MenuDefs;
	std::string WidgetVariant;
};
} // namespace game

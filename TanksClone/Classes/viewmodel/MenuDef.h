#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"

namespace tanks::viewmodel
{
/** One button of a menu. Texts are strings keys; Action, ValueId, When and EnabledWhen are ids the owning scene gives meaning to. */
struct MenuItemDef
{
	std::string Id;
	std::string Kind;
	std::string CheckedWhen;
	std::vector<std::string> Options;
	std::string OptionsSource;
	std::string OffText;
	std::string Suffix;
	float Min = 0.0f;
	float Max = 0.0f;
	float Step = 0.0f;
	std::string TextKey;
	std::string ValueId;
	std::string Sample;
	std::string Action;
	std::string When;
	std::string EnabledWhen;
	std::string Slot;
	bool bDebug = false;

	static MenuItemDef FromConfig(const model::IConfigNode& InNode);
	model::JsonValue ToConfig() const;
};

/** A text of the menu's root widget: the strings key the game sets, or a sample the editor shows for a text the code builds. */
struct MenuTextDef
{
	std::string Child;
	std::string Key;
	std::string Sample;
};

/** The size of the buttons of a menu and the gap between them; a 0 means the value of the menu's button style. */
struct MenuButtonMetrics
{
	float Width = 0.0f;
	float Height = 0.0f;
	float Spacing = 0.0f;
};

/**
 * A menu (configs/ui/menus/<name>.json): the root widget, its texts and the list of buttons with their order,
 * conditions and debug marks. The scenes build their buttons from it; the widget editor shows all of them.
 */
struct MenuDef
{
	std::string Id;
	std::string Root;
	std::string Container;
	std::string ButtonStyle;
	MenuButtonMetrics Button;
	std::vector<MenuTextDef> Texts;
	std::vector<MenuItemDef> Items;

	static MenuDef FromConfig(const model::IConfigNode& InNode, const std::string& InId);
	model::JsonValue ToConfig() const;
	const MenuTextDef* FindTextDef(const std::string& InChild) const;
};
} // namespace tanks::viewmodel

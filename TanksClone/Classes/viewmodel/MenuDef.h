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
	// How the row looks and behaves: "" (a plain button), "choice" (a value picked from a list with < >), "toggle" (a checkbox), "stepper" (a number with - +).
	std::string Kind;
	// A "toggle" row: a condition id that holds while the box is checked.
	std::string CheckedWhen;
	// A "choice" row: the strings ids (or literals) it steps through; empty when the list is owned elsewhere (see OptionsSource).
	std::vector<std::string> Options;
	// Where a list that is not in Options comes from ("languages", "difficulty", "display.fps_options"): shown by the editor, read by the owner.
	std::string OptionsSource;
	// A "stepper" row: its limits and step (the shown value is a number, e.g. a percent).
	float Min = 0.0f;
	float Max = 0.0f;
	float Step = 0.0f;
	std::string TextKey;
	// The caption reads "<text>: <value>" with the value the scene gives for this id; the editor shows Sample instead.
	std::string ValueId;
	std::string Sample;
	std::string Action;
	// A condition id: the button exists only while it holds. Empty = always.
	std::string When;
	// A condition id: the button is greyed out while it does not hold.
	std::string EnabledWhen;
	// Names a group of buttons the scene adds here (the extra pause buttons of a level); the item itself has no caption.
	std::string Slot;
	// Shown only in builds with debug tools (and always in the editor).
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

/**
 * A menu (configs/ui/menus/<name>.json): the root widget, its texts and the list of buttons with their order,
 * conditions and debug marks. The scenes build their buttons from it; the widget editor shows all of them.
 */
struct MenuDef
{
	std::string Id;
	std::string Root;
	// Where the buttons go (a child path of the root widget, '/' between nested widgets).
	std::string Container;
	// Which button size set applies: "main", "settings" or "overlay".
	std::string ButtonStyle;
	std::vector<MenuTextDef> Texts;
	std::vector<MenuItemDef> Items;

	static MenuDef FromConfig(const model::IConfigNode& InNode, const std::string& InId);
	model::JsonValue ToConfig() const;
	const MenuTextDef* FindTextDef(const std::string& InChild) const;
};

} // namespace tanks::viewmodel

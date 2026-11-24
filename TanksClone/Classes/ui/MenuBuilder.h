#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "viewmodel/MenuDef.h"

namespace tanks::view::cocos
{
/** A setting the menu file steps through by number: the current value and how to set a new one. */
struct MenuNumberBinding
{
	std::function<int()> Get;
	std::function<void(int)> Set;
};

/** A "choice" whose options are values in the menu file: the current option text and how to apply a picked one. */
struct MenuChoiceBinding
{
	std::function<std::string()> Get;
	std::function<void(const std::string&)> Set;
};

/** What a scene gives the menu data meaning with: the code behind each id the menu file names. */
struct MenuBindings
{
	std::map<std::string, std::function<void()>> Actions;
	std::map<std::string, std::function<void(int)>> Adjusts;
	std::map<std::string, std::function<std::string()>> Values;
	std::map<std::string, MenuNumberBinding> Indexes;
	std::map<std::string, MenuChoiceBinding> Choices;
	std::map<std::string, MenuNumberBinding> Numbers;
	std::map<std::string, std::function<bool()>> Conditions;
	std::map<std::string, std::vector<std::pair<std::string, std::function<void()>>>> Slots;
};

/** A button of a menu, resolved: its caption now, its action and whether it can be pressed. */
struct MenuEntry
{
	const viewmodel::MenuItemDef* Item = nullptr;
	std::string Caption;
	std::function<void()> Action;
	std::function<void(int)> Adjust;
	bool bEnabled = true;
	bool bChecked = false;
};

/**
 * Turns a menu file into the buttons to show. In the game, buttons whose "when" does not hold (and debug buttons of a
 * build without debug tools) are left out; in the editor (bInEditor) every button is listed with its sample caption and
 * does nothing.
 */
std::vector<MenuEntry> ResolveMenuEntries(const viewmodel::MenuDef& InMenu, const MenuBindings& InBindings, bool bInEditor);

bool MenuChecked(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, bool bInEditor);

std::string MenuCaption(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, bool bInEditor);

std::string MenuText(const viewmodel::MenuTextDef& InText);
} // namespace tanks::view::cocos

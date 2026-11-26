#pragma once

#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include <map>
#include <string>
#include <utility>

#include "viewmodel/MenuDef.h"

namespace game::view::cocos
{
/** A setting the menu file steps through by number: the current value and how to set a new one. */
struct MenuNumberBinding
{
	TFunction<int()> Get;
	TFunction<void(int), UiCallbackInlineSize> Set;
};

/** A "choice" whose options are values in the menu file: the current option text and how to apply a picked one. */
struct MenuChoiceBinding
{
	TFunction<std::string()> Get;
	TFunction<void(const std::string&), UiCallbackInlineSize> Set;
};

/** What a scene gives the menu data meaning with: the code behind each id the menu file names. */
struct MenuBindings
{
	TMap<FPersistentString, TFunction<void(), UiCallbackInlineSize>> Actions;
	TMap<FPersistentString, TFunction<void(int), UiCallbackInlineSize>> Adjusts;
	TMap<FPersistentString, TFunction<std::string()>> Values;
	TMap<FPersistentString, MenuNumberBinding> Indexes;
	TMap<FPersistentString, MenuChoiceBinding> Choices;
	TMap<FPersistentString, MenuNumberBinding> Numbers;
	TMap<FPersistentString, TFunction<bool()>> Conditions;
	TMap<FPersistentString, TArray<std::pair<std::string, TFunction<void(), UiCallbackInlineSize>>>> Slots;
};

/** A button of a menu, resolved: its caption now, its action and whether it can be pressed. */
struct MenuEntry
{
	const viewmodel::MenuItemDef* Item = nullptr;
	std::string Caption;
	TFunction<void(), UiCallbackInlineSize> Action;
	TFunction<void(int), UiCallbackInlineSize> Adjust;
	bool bEnabled = true;
	bool bChecked = false;
	TFunction<void(float), UiCallbackInlineSize> Slide;
};

/**
 * Turns a menu file into the buttons to show. In the game, buttons whose "when" does not hold (and debug buttons of a
 * build without debug tools) are left out; in the editor (bInEditor) every button is listed with its sample caption and
 * does nothing.
 */
TArray<MenuEntry> ResolveMenuEntries(const viewmodel::MenuDef& InMenu, const MenuBindings& InBindings, bool bInEditor);

bool MenuChecked(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, bool bInEditor);

std::string MenuCaption(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, bool bInEditor);

std::string MenuText(const viewmodel::MenuTextDef& InText);
} // namespace game::view::cocos

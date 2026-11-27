#pragma once

#include "templates/TFunction.h"
#include <string>

#include "cocos2d.h"

#include "ui/MenuBuilder.h"
#include "viewmodel/MenuDef.h"
#include "templates/TContainers.h"

namespace game
{
struct PlayerProfile;
}

namespace game::view::cocos
{
class LayoutWidget;

namespace widgets
{
class Button;
class ButtonList;
}

/**
 * The list of settings shared by the settings screen and the pause menu of a level: one button per setting, in the order of configs/ui/menus/settings.json.
 * A press changes the profile, applies and saves it, then refreshes every caption in place (no rebuild, so it
 * also works while the game is paused and the scheduler stands still).
 * Over a running level the settings that need the level reloaded (texture quality) are greyed out.
 * The text size changes the layout, so the owner rebuilds the panel (InOnReopen).
 */
class SettingsPanel : public cocos2d::Node
{
public:
	enum class Place
	{
		Screen,
		Level
	};

	static SettingsPanel* create(Place InPlace, const TFunction<void(), UiCallbackInlineSize>& InOnBack, const TFunction<void(), UiCallbackInlineSize>& InOnReopen);

	void Refresh();
	bool IsTextureQualityEnabled() const;

private:
	using ProfileChange = TFunction<void(PlayerProfile&), UiCallbackInlineSize>;

	struct Entry
	{
		widgets::Button* Target = nullptr;
		viewmodel::MenuItemDef Item;
	};

	bool init(Place InPlace, const TFunction<void(), UiCallbackInlineSize>& InOnBack, const TFunction<void(), UiCallbackInlineSize>& InOnReopen);
	void ShowFill(widgets::Button& InOutButton, const viewmodel::MenuItemDef& InItem) const;
	MenuBindings MakeBindings(const TFunction<void(), UiCallbackInlineSize>& InOnBack, const TFunction<void(), UiCallbackInlineSize>& InOnReopen);
	void AddEntry(const MenuEntry& InEntry);
	void BuildContent();
	void UpdateDescription();
	void ShowTab(int InIndex);
	void Apply(const ProfileChange& InChange, bool bInWindowChange);
	void Layout();

	Place Where = Place::Screen;
	LayoutWidget* Panel = nullptr;
	widgets::ButtonList* List = nullptr;
	LayoutWidget* Description = nullptr;
	widgets::Button* TextureButton = nullptr;
	MenuBindings Bindings;
	TArray<Entry> Entries;
	TArray<std::string> TabIds;
	TFunction<void(), UiCallbackInlineSize> OnReopen;
	const viewmodel::MenuDef* MenuDefinition = nullptr;
};
} // namespace game::view::cocos

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "cocos2d.h"

namespace tanks
{
struct PlayerProfile;
}

namespace tanks::view::cocos
{

class LayoutWidget;

namespace widgets
{
class Button;
class ButtonList;
}

/**
 * The list of settings shared by the settings screen and the pause menu of a level: one button per setting.
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
		Screen, // the settings screen of the menu
		Level   // over a paused level
	};

	static SettingsPanel* create(Place InPlace, const std::function<void()>& InOnBack, const std::function<void()>& InOnReopen);

	// Rewrites the title and every caption from the profile (after a change or a language switch).
	void Refresh();
	// The setting that needs the level reloaded; disabled over a level.
	bool IsTextureQualityEnabled() const;

private:
	using ProfileChange = std::function<void(PlayerProfile&)>;

	struct Entry
	{
		widgets::Button* Target = nullptr;
		std::function<std::string()> Caption;
	};

	bool init(Place InPlace, const std::function<void()>& InOnBack, const std::function<void()>& InOnReopen);
	void AddEntry(const std::function<std::string()>& InCaption, const std::function<void()>& InAction, bool bInEnabled = true);
	void AddSetting(const std::function<std::string()>& InCaption, const ProfileChange& InChange, bool bInWindowChange = false, bool bInEnabled = true);
	void Apply(const ProfileChange& InChange, bool bInWindowChange);
	void Layout();

	Place Where = Place::Screen;
	LayoutWidget* Panel = nullptr;
	widgets::ButtonList* List = nullptr;
	widgets::Button* TextureButton = nullptr;
	std::vector<Entry> Entries;
};

} // namespace tanks::view::cocos

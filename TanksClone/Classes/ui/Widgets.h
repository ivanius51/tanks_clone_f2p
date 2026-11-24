#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cocos2d.h"

#include "viewmodel/MenuDef.h"

namespace tanks::view::cocos
{
class LayoutWidget;
struct MenuBindings;

namespace widgets
{

// The shared colors of configs/ui/widgets.json.
namespace colors
{
	cocos2d::Color3B TextNormal();
	cocos2d::Color3B TextDim();
	cocos2d::Color3B Accent();
	cocos2d::Color3B Good();
	cocos2d::Color3B Bad();
	cocos2d::Color4F Panel();
	cocos2d::Color4F PanelBorder();
	cocos2d::Color4F ButtonColor();
	cocos2d::Color4F ButtonPressed();
	cocos2d::Color4F ButtonDisabled();
	cocos2d::Color4F ButtonBorder();
	cocos2d::Color4F Selection();
	cocos2d::Color4B Outline();
}

// How much text, buttons and their spacing grow for the player's text size (1 = the design size).
float GetUiScale();
cocos2d::Size ScaleUi(const cocos2d::Size& InSize);
// The button width, height and spacing of a menu: its own "button" numbers, else its style's ("main", "settings", "overlay"; InMenu may be null).
viewmodel::MenuButtonMetrics GetMenuButtonMetrics(const viewmodel::MenuDef* InMenu, std::string_view InStyle);
// The button size of a menu file's "button_style" ("main", "settings", "overlay"); InPanel is the menu's root widget.
cocos2d::Size GetMenuButtonSize(const viewmodel::MenuDef& InMenu, const LayoutWidget& InPanel);

// A text label; the size is scaled by GetUiScale(). Decorative labels (titles, banners) use the pixel font, everything else a plain font.
cocos2d::Label* CreateLabel(const std::string& InText, float InFontSize,
	const cocos2d::Color3B& InColor = colors::TextNormal(), bool bInDecorative = false);

// Outlines a label with the shared outline color and width.
void AddOutline(cocos2d::Label* InLabel);

// A dark background with a faint grid for the menu screens.
cocos2d::Node* CreateScreenBackground();
// The bottom line naming the menu buttons of the device used last ("A Select  B Back"); it follows device changes.
// A scene's line hides while an overlay is up, so only the overlay's own line shows.
cocos2d::Node* CreateControlHints(bool bInForOverlay = false);

// A sprite showing a frame of a loaded atlas; nullptr if no such frame is loaded.
cocos2d::Sprite* CreateFrameSprite(const std::string& InFrame);

// Calls InOnBack on the UiBack action (Esc, a phone's Back) while InOwner is on screen. Ignored while an overlay
// is shown, unless InOwner belongs to that overlay.
void AddBackKeyHandler(cocos2d::Node* InOwner, const std::function<void()>& InOnBack, bool bInOwnerIsOverlay = false);

// A clickable rectangle with a caption. Responds to mouse/touch and to ButtonList.
class Button : public cocos2d::Node
{
public:
	static Button* create(const std::string& InText, const cocos2d::Size& InSize,
		const std::function<void()>& InCallback);

	// What a menu row's controls look like: "<" ">" zones, "-" "+" zones, or a check box at the left edge.
	enum class Control
	{
		None,
		Choice,
		Stepper,
		Toggle
	};
	// The control of a menu item kind ("choice", "stepper", "toggle"); None for any other.
	static Control ControlFromKind(std::string_view InKind);
	// Draws the controls and makes their zones clickable: the left zone adjusts by -1, the right one by +1.
	void SetControl(Control InControl, bool bInChecked);

	void SetText(const std::string& InText);
	// Resizes the button and re-places its caption.
	void SetSize(const cocos2d::Size& InSize);

	// The Button behind InNode, or nullptr. Live buttons are registered, so no dynamic_cast is needed.
	static Button* FromNode(const cocos2d::Node* InNode);
	~Button() override;
	std::string GetText() const;
	void SetSelected(bool bInSelected);
	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	// Same as a click.
	void Activate();
	// A setting that can be stepped down (-1) or up (+1) besides activating.
	void SetAdjust(const std::function<void(int)>& InAdjust);
	void Adjust(int InDirection);

private:
	bool init(const std::string& InText, const cocos2d::Size& InSize, const std::function<void()>& InCallback);
	void Redraw();
	void DrawControls();
	void UpdateContentSize();
	// -1 the left zone, +1 the right zone, 0 the middle (or no zones).
	int ZoneAt(const cocos2d::Vec2& InWorld) const;
	bool IsTouchInside(cocos2d::Touch* InTouch) const;
	bool IsShown() const;

	cocos2d::DrawNode* Background = nullptr;
	LayoutWidget* Content = nullptr;
	std::function<void()> Callback;
	std::function<void(int)> AdjustCallback;
	bool bSelected = false;
	bool bEnabled = true;
	bool bPressed = false;
	Control CurrentControl = Control::None;
	bool bChecked = false;
	int HoverZone = 0;
	int PressedZone = 0;
};

// Keeps track of which button is selected and moves the selection with the keyboard
// (up/down or W/S, Enter or Space to press).
class ButtonList : public cocos2d::Node
{
public:
	CREATE_FUNC(ButtonList);

	// The list does not own or position the buttons: add them to the scene yourself.
	void AddButton(Button* InButton);
	// Stacks the buttons under each other, centered on InCenter (in the list's parent space).
	void LayoutVertically(const cocos2d::Vec2& InCenter, float InSpacing);

	void Select(int InIndex);
	int GetSelected() const;
	void Clear();

	// Lists inside an Overlay keep working while the overlay is up; all others are paused.
	void SetBelongsToOverlay(bool bInBelongs);

	bool init() override;

private:
	void MoveSelection(int InStep);

	std::vector<Button*> Buttons;
	int Selected = 0;
	bool bBelongsToOverlay = false;
};

// A dimmed full-screen layer with a panel, a title, a message and a column of buttons.
// It swallows all input, so the screen below cannot be clicked.
class Overlay : public cocos2d::LayerColor
{
public:
	typedef std::pair<std::string, std::function<void()>> ButtonSpec;

	// InButtonHeight 0 takes the height of the widget style.
	static Overlay* create(const std::string& InTitle, const std::string& InMessage,
		const std::vector<ButtonSpec>& InButtons, float InButtonHeight = 0.0f);
	// An overlay whose title, message and buttons come from the menu file configs/ui/menus/<InMenuId>.json.
	static Overlay* CreateFromMenu(std::string_view InMenuId, const MenuBindings& InBindings);
	// Only the dimmed, input-swallowing layer: the caller adds its own content.
	static Overlay* CreateEmpty();

	// True while at least one overlay is on screen.
	static bool IsShown();

	void onEnter() override;
	void onExit() override;

private:
	bool InitLayer();
	bool init(const std::string& InTitle, const std::string& InMessage, const std::vector<ButtonSpec>& InButtons, float InButtonHeight);
};

} // namespace widgets
} // namespace tanks::view::cocos

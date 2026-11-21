#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "cocos2d.h"

namespace tanks::view::cocos
{
namespace widgets
{

namespace colors
{
	const cocos2d::Color3B TextNormal(235, 235, 235);
	const cocos2d::Color3B TextDim(150, 150, 160);
	const cocos2d::Color3B Accent(255, 205, 70);
	const cocos2d::Color3B Good(110, 220, 110);
	const cocos2d::Color3B Bad(235, 90, 80);
	const cocos2d::Color4F Panel(0.08f, 0.09f, 0.12f, 0.96f);
	const cocos2d::Color4F PanelBorder(0.45f, 0.5f, 0.6f, 1.0f);
	const cocos2d::Color4F ButtonColor(0.16f, 0.19f, 0.26f, 1.0f);
	const cocos2d::Color4F ButtonPressed(0.10f, 0.12f, 0.17f, 1.0f);
	const cocos2d::Color4F ButtonDisabled(0.11f, 0.12f, 0.15f, 1.0f);
	const cocos2d::Color4F ButtonBorder(0.35f, 0.4f, 0.5f, 1.0f);
	const cocos2d::Color4F Selection(1.0f, 0.8f, 0.25f, 1.0f);
}

// A text label. Decorative labels (titles, banners) use the pixel font, everything else a plain font.
cocos2d::Label* CreateLabel(const std::string& InText, float InFontSize,
	const cocos2d::Color3B& InColor = colors::TextNormal, bool bInDecorative = false);

// A dark background with a faint grid for the menu screens.
cocos2d::Node* CreateScreenBackground();

// A sprite showing a frame of a loaded atlas; nullptr if no such frame is loaded.
cocos2d::Sprite* CreateFrameSprite(const std::string& InFrame);

// Calls InOnBack when Esc (PC) or Back (phone) is pressed while InOwner is on screen. Ignored while an overlay
// is shown, unless InOwner belongs to that overlay.
void AddBackKeyHandler(cocos2d::Node* InOwner, const std::function<void()>& InOnBack, bool bInOwnerIsOverlay = false);

// A clickable rectangle with a caption. Responds to mouse/touch and to ButtonList.
class Button : public cocos2d::Node
{
public:
	static Button* create(const std::string& InText, const cocos2d::Size& InSize,
		const std::function<void()>& InCallback);

	void SetText(const std::string& InText);

	// The Button behind InNode, or nullptr. Live buttons are registered, so no dynamic_cast is needed.
	static Button* FromNode(const cocos2d::Node* InNode);
	~Button() override;
	std::string GetText() const;
	void SetSelected(bool bInSelected);
	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	// Same as a click.
	void Activate();

private:
	bool init(const std::string& InText, const cocos2d::Size& InSize, const std::function<void()>& InCallback);
	void Redraw();
	bool IsTouchInside(cocos2d::Touch* InTouch) const;
	bool IsShown() const;

	cocos2d::DrawNode* Background = nullptr;
	cocos2d::Label* TextLabel = nullptr;
	std::function<void()> Callback;
	bool bSelected = false;
	bool bEnabled = true;
	bool bPressed = false;
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

	static Overlay* create(const std::string& InTitle, const std::string& InMessage,
		const std::vector<ButtonSpec>& InButtons);
	// Only the dimmed, input-swallowing layer: the caller adds its own content.
	static Overlay* CreateEmpty();

	// True while at least one overlay is on screen.
	static bool IsShown();

	void onEnter() override;
	void onExit() override;

private:
	bool InitLayer();
	bool init(const std::string& InTitle, const std::string& InMessage, const std::vector<ButtonSpec>& InButtons);
};

} // namespace widgets
} // namespace tanks::view::cocos

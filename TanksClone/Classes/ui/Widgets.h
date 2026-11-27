#pragma once

#include "templates/TFunction.h"
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "cocos2d.h"

#include "core/ResourceIds.h"
#include "viewmodel/MenuDef.h"
#include "viewmodel/WidgetStyle.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{
class LayoutWidget;
struct MenuBindings;
struct MenuEntry;

namespace widgets
{
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

float GetUiScale();
cocos2d::Size ScaleUi(const cocos2d::Size& InSize);
viewmodel::MenuButtonMetrics GetMenuButtonMetrics(const viewmodel::MenuDef* InMenu, std::string_view InStyle);
cocos2d::Size GetMenuButtonSize(const viewmodel::MenuDef& InMenu, const LayoutWidget& InPanel);
// A button for a resolved menu entry: the menu's default size unless the item gives a height, plus the item's style, font size and subtitle.
class Button* CreateEntryButton(const MenuEntry& InEntry, const cocos2d::Size& InDefaultSize);

cocos2d::Label* CreateLabel(const std::string& InText, float InFontSize,
	const cocos2d::Color3B& InColor = colors::TextNormal(), bool bInDecorative = false);

void AddOutline(cocos2d::Label* InLabel);

cocos2d::Node* CreateScreenBackground();
cocos2d::Node* CreateControlHints(bool bInForOverlay = false);
void SetTabsHint(bool bInShown);

cocos2d::Sprite* CreateFrameSprite(const std::string& InFrame);

void AddBackKeyHandler(cocos2d::Node* InOwner, const TFunction<void(), UiCallbackInlineSize>& InOnBack, bool bInOwnerIsOverlay = false);

/** A clickable rectangle with a caption. Responds to mouse/touch and to ButtonList. */
class Button : public cocos2d::Node
{
public:
	static Button* create(const std::string& InText, const cocos2d::Size& InSize,
		const TFunction<void(), UiCallbackInlineSize>& InCallback);

	enum class Control
	{
		None,
		Choice,
		Stepper,
		Slider,
		Toggle
	};
	static Control ControlFromKind(std::string_view InKind);
	void SetControl(Control InControl, bool bInChecked);

	void SetText(const std::string& InText);
	void SetSize(const cocos2d::Size& InSize);
	// A style of configs/ui/widgets.json "button_styles"; an unknown id or none is the default style.
	void SetStyle(std::string_view InStyleId);
	// Replaces the fill of the style with this color (and a darker one while pressed).
	void SetMainColor(const model::Color& InColor);
	// A smaller dim line under the caption; empty removes it.
	void SetSubtitle(const std::string& InText);
	void SetFontSize(float InFontSize);
	// Shows "Name: Value" captions as the name on the left and the value on the right.
	void SetRowMode(bool bInRow);

	static Button* FromNode(const cocos2d::Node* InNode);
	~Button() override;
	std::string GetText() const;
	void SetSelected(bool bInSelected);
	void SetActive(bool bInActive);
	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	void Activate();
	bool IsUnderCursor(const cocos2d::Vec2& InWorld) const;
	void SetAdjust(const TFunction<void(int), UiCallbackInlineSize>& InAdjust);
	void Adjust(int InDirection);
	void SetSlide(const TFunction<void(float), UiCallbackInlineSize>& InSlide);
	void SetFillFraction(float InFraction);

private:
	bool init(const std::string& InText, const cocos2d::Size& InSize, const TFunction<void(), UiCallbackInlineSize>& InCallback);
	void Redraw();
	void DrawControls();
	void SlideTo(const cocos2d::Touch* InTouch);
	void UpdateContentSize();
	void LayoutRow();
	int ZoneAt(const cocos2d::Vec2& InWorld) const;
	bool IsTouchInside(cocos2d::Touch* InTouch) const;
	bool IsShown() const;

	viewmodel::ButtonStyle GetEffectiveStyle() const;

	cocos2d::DrawNode* Background = nullptr;
	LayoutWidget* Content = nullptr;
	cocos2d::Label* RowName = nullptr;
	cocos2d::Label* RowValue = nullptr;
	float RowFontSize = 22.0f;
	std::string StyleId;
	std::optional<model::Color> MainColor;
	std::string Subtitle;
	TFunction<void(), UiCallbackInlineSize> Callback;
	TFunction<void(int), UiCallbackInlineSize> AdjustCallback;
	TFunction<void(float), UiCallbackInlineSize> SlideCallback;
	float FillFraction = 0.0f;
	bool bSelected = false;
	bool bActive = false;
	bool bEnabled = true;
	bool bPressed = false;
	Control CurrentControl = Control::None;
	bool bChecked = false;
	int HoverZone = 0;
	int PressedZone = 0;
};

/**
 * Keeps track of which button is selected and moves the selection with the keyboard
 * (up/down or W/S, Enter or Space to press).
 */
class ButtonList : public cocos2d::Node
{
public:
	CREATE_FUNC(ButtonList);

	void AddButton(Button* InButton);
	void LayoutVertically(const cocos2d::Vec2& InCenter, float InSpacing);

	void Select(int InIndex);
	int GetSelected() const;
	void Clear();

	void SetBelongsToOverlay(bool bInBelongs);

	bool init() override;

private:
	void MoveSelection(int InStep);

	TArray<Button*> Buttons;
	int Selected = 0;
	bool bBelongsToOverlay = false;
};

/**
 * A dimmed full-screen layer with a panel, a title, a message and a column of buttons.
 * It swallows all input, so the screen below cannot be clicked.
 */
class Overlay : public cocos2d::LayerColor
{
public:
	typedef std::pair<std::string, TFunction<void(), UiCallbackInlineSize>> ButtonSpec;

	static Overlay* create(const std::string& InTitle, const std::string& InMessage,
		const TArray<ButtonSpec>& InButtons, float InButtonHeight = 0.0f, std::string_view InWidgetId = ids::widgets::Overlay);
	static Overlay* CreateFromMenu(std::string_view InMenuId, const MenuBindings& InBindings);
	static Overlay* CreateEmpty();

	static bool IsShown();

	void onEnter() override;
	void onExit() override;

private:
	bool InitLayer();
	bool init(const std::string& InTitle, const std::string& InMessage, const TArray<ButtonSpec>& InButtons, float InButtonHeight, std::string_view InWidgetId);
};
} // namespace widgets
} // namespace game::view::cocos

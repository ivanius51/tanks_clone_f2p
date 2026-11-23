#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "model/Animation.h"
#include "model/BarMeter.h"
#include "viewmodel/WidgetDef.h"

namespace tanks::view::cocos
{

/**
 * A widget built from a widget file (configs/ui/widgets/<name>.json) by the WidgetFactory: a node whose children are
 * labels, icons, rectangles, spacers and other widgets, placed by the layout rules of the file (row, column, anchor,
 * stack). Its origin is its centre. Whenever a text, a frame or the size changes, the widget measures its children and
 * places them again, and so does every widget around it, so nothing stays at a stale position.
 */
class LayoutWidget : public cocos2d::Node
{
public:
	using CommandHandler = std::function<void(std::string_view)>;

	// nullptr (and logged) when no widget file has this id.
	static LayoutWidget* Create(std::string_view InId);
	static LayoutWidget* Create(const viewmodel::WidgetDef& InDef);

	// The child's text; an unknown child is ignored.
	void SetText(std::string_view InChild, const std::string& InText);
	std::string GetText(std::string_view InChild) const;
	void SetTextColor(std::string_view InChild, const cocos2d::Color3B& InColor);
	void SetIconFrame(std::string_view InChild, const std::string& InFrame);
	// Replaces the file's "size" (0 on an axis = as big as the content).
	void SetFixedSize(const cocos2d::Size& InSize);
	// The width left for children: the fixed width minus the padding on both sides, at the current text scale.
	float GetInnerWidth() const;
	// The space between children at the current text scale.
	float GetGap() const;
	// Replaces the file's "gap" (a negative value goes back to the file's).
	void SetGap(float InGap);
	// Dims a child (a disabled tab, a menu item that does not apply); 1 is opaque.
	void SetPartOpacity(std::string_view InChild, float InOpacity);
	// Recolours a rectangle or ring child by shared color names ("selection", "button"); an empty name keeps the current one.
	void SetPartColors(std::string_view InChild, std::string_view InFill, std::string_view InBorder);
	// A bar child: the filled share, 0..1. The first value is shown at once; later ones ease in (anim_time) and a drop leaves a draining trail.
	void SetPartFraction(std::string_view InChild, float InFraction);
	// The share a bar child draws right now (it follows the set one while animating).
	float GetPartShownFraction(std::string_view InChild) const;
	// A rectangle, ring or bar child: the fill color as given (the low-health red of a bar).
	void SetPartFillColor(std::string_view InChild, const model::Color& InColor);
	// A hidden child takes no room and no gap.
	void SetPartVisible(std::string_view InChild, bool bInVisible);
	// Starts the animation file (cfg.anim.<name>) on a child, replacing the one it had; an empty id stops it.
	void PlayPartAnimation(std::string_view InChild, std::string_view InAnimation);
	// The pointer is over this widget: children with hover_fill, hover_border or hover_opacity show those.
	void SetHovered(bool bInHovered);
	bool IsHovered() const;
	// True when the widget reacts to the pointer (it has a click command or hover keys).
	bool IsHoverable() const;
	int GetHoverLayer() const;
	// The widget's box in world coordinates, for hit testing.
	model::RectF GetWorldBounds() const;

	void onEnter() override;
	void onExit() override;

	// Adds a node the code made (a button) as the last child; the layout measures it by its content size.
	void AppendNode(cocos2d::Node* InNode);
	void RemoveAppendedNodes();

	cocos2d::Label* FindLabel(std::string_view InChild) const;
	cocos2d::Node* FindPart(std::string_view InChild) const;
	LayoutWidget* FindWidget(std::string_view InChild) const;

	void update(float InDelta) override;

	// Takes the widget file again (after UiStyle::ReloadWidgets) and rebuilds every widget on screen that came from one.
	static void ReloadAll();

	// Builds every child again (after the font, the language or the text size changed), keeping set texts.
	void Rebuild();
	// Measures and places the children again, here and in every widget around this one.
	void Relayout();

	const viewmodel::WidgetDef& GetDef() const;
	// Runs the owner's handler with the file's on_click command id; false when there is none.
	bool Activate() const;
	void SetCommandHandler(const CommandHandler& InHandler);

private:
	struct Part
	{
		viewmodel::WidgetChildDef Def;
		cocos2d::Node* Node = nullptr;
		cocos2d::Label* TextLabel = nullptr;
		std::string FullText;
		cocos2d::DrawNode* Drawing = nullptr;
		cocos2d::Sprite* Picture = nullptr;
		LayoutWidget* Sub = nullptr;
		cocos2d::Size Size;
		model::BarMeter Meter;
		bool bAppended = false;
		std::optional<model::AnimationPlayer> Player;
		cocos2d::Vec2 LayoutPosition;
		float BaseScaleX = 1.0f;
		float BaseScaleY = 1.0f;
		bool bHovered = false;
	};

	void ApplyAnimation(Part& InOutPart);
	void StartAnimation(Part& InOutPart, std::string_view InAnimation);
	const viewmodel::ColorRef& FillOf(const Part& InPart) const;
	const viewmodel::ColorRef& BorderOf(const Part& InPart) const;

	static bool ShrinksText(const Part& InPart);
	void FitText(Part& InOutPart, float InAvailableWidth) const;

	bool init(const viewmodel::WidgetDef& InDef);
	void Build();
	void BuildPart(const viewmodel::WidgetChildDef& InDef);
	Part* FindEntry(std::string_view InChild);
	const Part* FindEntry(std::string_view InChild) const;
	LayoutWidget* GetRoot();
	cocos2d::Size Arrange();
	cocos2d::Size Measure(Part& InOutPart, float InAvailableWidth) const;
	void Resize(Part& InOutPart, const cocos2d::Size& InSize);
	void DrawRect(Part& InOutPart);
	void DrawRing(Part& InOutPart);
	void DrawBar(Part& InOutPart);
	float GetScale() const;

	viewmodel::WidgetDef Def;
	std::vector<Part> Parts;
	std::map<std::string, std::string> Texts;
	LayoutWidget* Outer = nullptr;
	~LayoutWidget() override;
	cocos2d::Size FixedOverride;
	float GapOverride = -1.0f;
	cocos2d::Vector<cocos2d::Node*> Appended;
	CommandHandler Handler;
	bool bHovered = false;
};

} // namespace tanks::view::cocos

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"
#include "model/WidgetLayout.h"

namespace tanks::viewmodel
{

/** A color a widget file gives either by the name of a shared color ("accent") or as [r, g, b, a]. */
struct ColorRef
{
	std::string Name;
	model::Color Literal;
	bool bLiteral = false;

	ColorRef() = default;
	explicit ColorRef(std::string InName) : Name(std::move(InName)) {}

	bool IsSet() const { return bLiteral || !Name.empty(); }
	bool FromConfig(const model::IConfigNode& InNode);
};

enum class WidgetPart
{
	Widget, // another widget file, by id
	Label,
	Icon,
	Rect,   // a filled, bordered rectangle
	Ring,   // a circular arrow: size is the diameter, border_width the line, fill the color
	Spacer  // an empty box of a fixed size
};

/** One entry of a widget's "children" list (configs/ui/widgets/<name>.json). */
struct WidgetChildDef
{
	// The name the owner finds this child by ("text", "icon"); may be empty.
	std::string Id;
	WidgetPart Part = WidgetPart::Spacer;
	// Part Widget: the id of the widget file.
	std::string Widget;
	// Part Label: a literal text and/or a strings key; the owner can replace the text at run time.
	std::string Text;
	std::string TextKey;
	// Part Icon: the sprite frame.
	std::string Frame;
	float FontSize = 0.0f;
	bool bDecorative = false;
	ColorRef TextColor{ "text" };
	ColorRef FillColor;
	ColorRef BorderColor;
	float BorderWidth = 0.0f;
	// A label wider than the room it gets shrinks to fit.
	bool bShrinkToFit = false;
	// A label breaks into lines at the width its container gives it, centred.
	bool bWrap = false;
	model::Vec2f Size;
	model::Vec2f Anchor{ 0.5f, 0.5f };
	model::Vec2f Offset;
	// Anchor layout: the point of this child that sits on the anchor (see model::LayoutChild).
	model::Vec2f Pivot;
	bool bHasPivot = false;
	// Part Rect: corner radius (a pill when it reaches half the height).
	float Radius = 0.0f;
	bool bFill = false;
	// Row and column: the share of the leftover main-axis room this child takes (0 = none).
	float Grow = 0.0f;
	// Row and column: takes the whole cross-axis size of the container.
	bool bStretch = false;
	// 0..1; a dimmed child shows a disabled or inactive state.
	float Opacity = 1.0f;
};

/** A widget file: its layout rules, children and what a click does. A "size" of 0 on an axis means as big as the content. */
struct WidgetDef
{
	std::string Id;
	model::LayoutRules Rules;
	std::vector<WidgetChildDef> Children;
	// A command id the owner maps to an action; empty for a widget that is not clickable.
	std::string OnClick;
	// Scales every measure of this widget (not the text, which follows the text size) when the player's text size changes.
	bool bScaleWithText = false;

	// InId names the widget; OutMissing gets the path of every required key that is missing.
	static WidgetDef FromConfig(const model::IConfigNode& InNode, const std::string& InId, std::vector<std::string>& OutMissing);
};

bool EnumFromName(std::string_view InName, WidgetPart& OutPart);

} // namespace tanks::viewmodel

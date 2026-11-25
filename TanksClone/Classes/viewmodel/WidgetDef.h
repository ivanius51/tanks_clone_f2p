#pragma once

#include <string>
#include <utility>
#include <vector>

#include "model/Animation.h"
#include "model/ConfigNode.h"
#include "model/JsonValue.h"
#include "model/MathTypes.h"
#include "model/WidgetLayout.h"

namespace game::viewmodel
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
	model::JsonValue ToConfig() const;
};

enum class WidgetPart
{
	Widget,
	Label,
	Icon,
	Rect,
	Ring,
	Bar,
	Spacer
};

/** One entry of a widget's "children" list (configs/ui/widgets/<name>.json). */
struct WidgetChildDef
{
	std::string Id;
	WidgetPart Part = WidgetPart::Spacer;
	std::string Widget;
	std::string Text;
	std::string TextKey;
	std::string Frame;
	float FontSize = 0.0f;
	bool bDecorative = false;
	ColorRef TextColor{ "text" };
	ColorRef FillColor;
	ColorRef BorderColor;
	ColorRef BackColor;
	float AnimTime = 0.0f;
	ColorRef TrailColor;
	float TrailDelay = 0.0f;
	model::Easing AnimEase = model::Easing::Linear;
	float BorderWidth = 0.0f;
	bool bShrinkToFit = false;
	float MaxWidth = 0.0f;
	float MinScale = 0.0f;
	int MaxChars = 0;
	bool bWrap = false;
	model::Vec2f Size;
	model::Vec2f Anchor{ 0.5f, 0.5f };
	model::Vec2f Offset;
	model::Vec2f Pivot;
	bool bHasPivot = false;
	float Radius = 0.0f;
	bool bFill = false;
	float Grow = 0.0f;
	bool bStretch = false;
	float Opacity = 1.0f;
	bool bVisible = true;
	float Rotation = 0.0f;
	float Scale = 1.0f;
	int ZOrder = 0;

	std::string Animation;
	ColorRef HoverFill;
	ColorRef HoverBorder;
	float HoverOpacity = -1.0f;

	model::JsonValue ToConfig() const;
};

/** A widget file: its layout rules, children and what a click does. A "size" of 0 on an axis means as big as the content. */
struct WidgetDef
{
	std::string Id;
	model::LayoutRules Rules;
	std::vector<WidgetChildDef> Children;
	std::string OnClick;
	bool bScaleWithText = false;
	int Layer = 0;
	model::JsonValue Variants;

	static WidgetDef FromConfig(const model::IConfigNode& InNode, const std::string& InId, std::vector<std::string>& OutMissing, std::string_view InVariant = {});
	model::JsonValue ToConfig() const;
};

bool EnumFromName(std::string_view InName, WidgetPart& OutPart);
} // namespace game::viewmodel

#pragma once

#include <array>
#include <string>
#include <utility>

#include "model/Animation.h"
#include "model/Gradient.h"
#include "model/ConfigNode.h"
#include "model/JsonValue.h"
#include "model/MathTypes.h"
#include "model/WidgetLayout.h"
#include "templates/TContainers.h"

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

/** A gradient a widget file gives as { "direction": "vertical", "stops": [color, ...] } with up to model::MaxGradientStops colors, each a shared color name or [r, g, b, a]. */
struct GradientRef
{
	model::GradientDirection Direction = model::GradientDirection::Vertical;
	std::array<ColorRef, model::MaxGradientStops> Stops;
	int Count = 0;

	bool IsSet() const { return Count >= 2; }
	bool FromConfig(const model::IConfigNode& InNode);
	model::JsonValue ToConfig() const;
};

enum class WidgetPart
{
	Widget,
	Label,
	Icon,
	Image,
	Rect,
	Ring,
	Bar,
	Glyph,
	Spacer
};

enum class WidgetGlyph
{
	ChevronLeft,
	ChevronRight,
	Plus,
	Minus
};

/**
 * One entry of a widget's "children" list (configs/ui/widgets/<name>.json). ShowIf names a flag of the owning widget
 * (LayoutWidget::SetFlag): the child shows unless that flag was set to false. Item names the widget a list child
 * repeats (LayoutWidget::SetListCount). ScaleX and ScaleY (when not negative) replace Scale on one axis; bTiled makes an icon repeat its frame over its size. SubTexts and SubTextKeys set labels inside a "widget" child by path
 * ("prev/text") to a literal text or a string id.
 */
struct WidgetChildDef
{
	std::string Id;
	WidgetPart Part = WidgetPart::Spacer;
	std::string Widget;
	std::string Text;
	std::string TextKey;
	std::string Frame;
	std::string Image;
	std::string Fit;
	float FontSize = 0.0f;
	bool bDecorative = false;
	ColorRef TextColor{ "text" };
	ColorRef FillColor;
	GradientRef FillGradient;
	ColorRef BorderColor;
	ColorRef BackColor;
	float AnimTime = 0.0f;
	ColorRef TrailColor;
	float TrailDelay = 0.0f;
	model::Easing AnimEase = model::Easing::Linear;
	WidgetGlyph Glyph = WidgetGlyph::ChevronLeft;
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
	bool bFillX = false;
	bool bFillY = false;
	bool bHasAlign = false;
	model::LayoutAlign Align = model::LayoutAlign::Center;
	float Opacity = 1.0f;
	bool bVisible = true;
	float Rotation = 0.0f;
	float Scale = 1.0f;
	float ScaleX = -1.0f;
	float ScaleY = -1.0f;
	bool bTiled = false;
	int ZOrder = 0;

	std::string Animation;
	std::string ShowIf;
	std::string Item;
	TArray<std::pair<std::string, std::string>> SubTexts;
	TArray<std::pair<std::string, std::string>> SubTextKeys;
	ColorRef HoverFill;
	ColorRef PressFill;
	ColorRef HoverBorder;
	float HoverOpacity = -1.0f;

	model::JsonValue ToConfig() const;
};

/** A widget file: its layout rules, children and what a click does. bClip cuts the children at its box, bTouchable false ignores clicks, bSlider makes it a slider (parts "fill" and "knob"). A "size" of 0 on an axis means as big as the content. */
struct WidgetDef
{
	std::string Id;
	model::LayoutRules Rules;
	TArray<WidgetChildDef> Children;
	std::string OnClick;
	bool bScaleWithText = false;
	bool bClip = false;
	bool bTouchable = true;
	bool bSlider = false;
	bool bScreen = false;
	int Layer = 0;
	model::JsonValue Variants;

	static WidgetDef FromConfig(const model::IConfigNode& InNode, const std::string& InId, TArray<std::string>& OutMissing, std::string_view InVariant = {});
	model::JsonValue ToConfig() const;
};

bool EnumFromName(std::string_view InName, WidgetPart& OutPart);
bool EnumFromName(std::string_view InName, WidgetGlyph& OutGlyph);
} // namespace game::viewmodel

#pragma once

#include <string_view>

#include "model/MathTypes.h"
#include "templates/TContainers.h"

namespace game::model
{
enum class LayoutKind
{
	Stack,
	Row,
	Column,
	Anchor,
	Grid
};

enum class LayoutAlign
{
	Start,
	Center,
	End
};

bool EnumFromName(std::string_view InName, LayoutKind& OutKind);
bool EnumFromName(std::string_view InName, LayoutAlign& OutAlign);

/** How one container places its children. A zero FixedSize axis means "as big as the content". */
struct LayoutRules
{
	LayoutKind Kind = LayoutKind::Stack;
	float Gap = 0.0f;
	Vec2f Padding;
	Vec2f FixedSize;
	Vec2f MinSize;
	LayoutAlign MainAlign = LayoutAlign::Center;
	LayoutAlign CrossAlign = LayoutAlign::Center;
	int Columns = 1;
	bool bEqualChildren = false;
};

/** One child to place: its measured Size and placement hints in; Position (its centre from the container centre, Y up) out. */
struct LayoutChild
{
	Vec2f Size;
	Vec2f Anchor{ 0.5f, 0.5f };
	Vec2f Pivot{ 0.5f, 0.5f };
	bool bHasPivot = false;
	Vec2f Offset;
	bool bFill = false;
	bool bHidden = false;
	float Grow = 0.0f;
	bool bStretch = false;
	Vec2f Position;
};

/** The pure layout algorithm of the widget factory: no engine types, so it is unit-tested directly. */
namespace widget_layout
{
Vec2f Arrange(const LayoutRules& InRules, TArray<LayoutChild>& InOutChildren);

float FitFactor(float InAvailable, float InFixed, float InScalable, float InMinFactor);
} // namespace widget_layout
} // namespace game::model

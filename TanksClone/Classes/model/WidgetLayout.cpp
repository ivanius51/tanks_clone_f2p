#include "model/WidgetLayout.h"

#include <algorithm>
#include <cmath>

namespace tanks::model
{

bool EnumFromName(const std::string_view InName, LayoutKind& OutKind)
{
	if ( InName == "stack" ) { OutKind = LayoutKind::Stack; return true; }
	if ( InName == "row" ) { OutKind = LayoutKind::Row; return true; }
	if ( InName == "column" ) { OutKind = LayoutKind::Column; return true; }
	if ( InName == "anchor" ) { OutKind = LayoutKind::Anchor; return true; }
	return false;
}

bool EnumFromName(const std::string_view InName, LayoutAlign& OutAlign)
{
	if ( InName == "start" ) { OutAlign = LayoutAlign::Start; return true; }
	if ( InName == "center" ) { OutAlign = LayoutAlign::Center; return true; }
	if ( InName == "end" ) { OutAlign = LayoutAlign::End; return true; }
	return false;
}

namespace widget_layout
{

namespace
{
	// The start of a block of InContent inside InInner, as an offset from the inner centre-line (centre of the block).
	float AlignedCenter(const LayoutAlign InAlign, const float InInner, const float InContent)
	{
		switch ( InAlign )
		{
			case LayoutAlign::Start: return -(InInner - InContent) / 2.0f;
			case LayoutAlign::End: return (InInner - InContent) / 2.0f;
			default: return 0.0f;
		}
	}

	Vec2f MeasureContent(const LayoutRules& InRules, const std::vector<LayoutChild>& InChildren)
	{
		Vec2f Content;
		int Counted = 0;
		for ( const LayoutChild& Child : InChildren )
		{
			if ( Child.bFill || Child.bHidden )
			{
				continue;
			}
			++Counted;
			switch ( InRules.Kind )
			{
				case LayoutKind::Row:
					Content.X += Child.Size.X;
					Content.Y = std::max(Content.Y, Child.Size.Y);
					break;
				case LayoutKind::Column:
					Content.Y += Child.Size.Y;
					Content.X = std::max(Content.X, Child.Size.X);
					break;
				default:
					Content.X = std::max(Content.X, std::abs(Child.Offset.X) * 2.0f + Child.Size.X);
					Content.Y = std::max(Content.Y, std::abs(Child.Offset.Y) * 2.0f + Child.Size.Y);
					break;
			}
		}
		if ( Counted > 1 )
		{
			const float Gaps = InRules.Gap * static_cast<float>(Counted - 1);
			if ( InRules.Kind == LayoutKind::Row )
			{
				Content.X += Gaps;
			}
			else if ( InRules.Kind == LayoutKind::Column )
			{
				Content.Y += Gaps;
			}
		}
		return Content;
	}
}

Vec2f Arrange(const LayoutRules& InRules, std::vector<LayoutChild>& InOutChildren)
{
	const Vec2f Content = MeasureContent(InRules, InOutChildren);
	Vec2f Size(InRules.FixedSize.X > 0.0f ? InRules.FixedSize.X : Content.X + InRules.Padding.X * 2.0f,
		InRules.FixedSize.Y > 0.0f ? InRules.FixedSize.Y : Content.Y + InRules.Padding.Y * 2.0f);
	Size.X = std::max(Size.X, InRules.MinSize.X);
	Size.Y = std::max(Size.Y, InRules.MinSize.Y);
	const Vec2f Inner(std::max(Size.X - InRules.Padding.X * 2.0f, 0.0f), std::max(Size.Y - InRules.Padding.Y * 2.0f, 0.0f));

	float Cursor = 0.0f;
	if ( InRules.Kind == LayoutKind::Row )
	{
		Cursor = -Content.X / 2.0f + AlignedCenter(InRules.MainAlign, Inner.X, Content.X);
	}
	else if ( InRules.Kind == LayoutKind::Column )
	{
		Cursor = Content.Y / 2.0f - AlignedCenter(InRules.MainAlign, Inner.Y, Content.Y);
	}

	for ( LayoutChild& Child : InOutChildren )
	{
		if ( Child.bHidden )
		{
			continue;
		}
		if ( Child.bFill )
		{
			Child.Size = Inner;
			Child.Position = Child.Offset;
			continue;
		}
		switch ( InRules.Kind )
		{
			case LayoutKind::Row:
				Child.Position = Vec2f(Cursor + Child.Size.X / 2.0f, -AlignedCenter(InRules.CrossAlign, Inner.Y, Child.Size.Y)) + Child.Offset;
				Cursor += Child.Size.X + InRules.Gap;
				break;
			case LayoutKind::Column:
				Child.Position = Vec2f(AlignedCenter(InRules.CrossAlign, Inner.X, Child.Size.X), Cursor - Child.Size.Y / 2.0f) + Child.Offset;
				Cursor -= Child.Size.Y + InRules.Gap;
				break;
			case LayoutKind::Anchor:
			{
				const Vec2f Point((Child.Anchor.X - 0.5f) * Inner.X, (Child.Anchor.Y - 0.5f) * Inner.Y);
				const Vec2f Pivot((Child.Anchor.X - 0.5f) * Child.Size.X, (Child.Anchor.Y - 0.5f) * Child.Size.Y);
				Child.Position = Point - Pivot + Child.Offset;
				break;
			}
			default:
				Child.Position = Child.Offset;
				break;
		}
	}
	return Size;
}

float FitFactor(const float InAvailable, const float InFixed, const float InScalable, const float InMinFactor)
{
	if ( InScalable <= 0.0f || InFixed + InScalable <= InAvailable )
	{
		return 1.0f;
	}
	return std::clamp((InAvailable - InFixed) / InScalable, InMinFactor, 1.0f);
}

} // namespace widget_layout

} // namespace tanks::model

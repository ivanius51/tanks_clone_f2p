#include "model/ScreenLayout.h"

#include <algorithm>
#include <array>

namespace tanks::model
{

bool EnumFromName(const std::string_view InName, ScreenAnchor& OutAnchor)
{
	constexpr std::array<std::string_view, 9> Names = { "bottom_left", "bottom", "bottom_right", "left", "center", "right", "top_left", "top", "top_right" };
	static_assert(static_cast<std::size_t>(ScreenAnchor::TopRight) + 1 == Names.size(), "one name per anchor, in order");
	for ( std::size_t Index = 0; Index < Names.size(); ++Index )
	{
		if ( Names[Index] == InName )
		{
			OutAnchor = static_cast<ScreenAnchor>(Index);
			return true;
		}
	}
	return false;
}

ScreenLayout::ScreenLayout(const Vec2f& InDesignSize, const Vec2f& InFrameSize)
	: DesignSize(InDesignSize), FrameSize(InFrameSize)
{
	const bool bValid = DesignSize.X > 0.0f && DesignSize.Y > 0.0f && FrameSize.X > 0.0f && FrameSize.Y > 0.0f;
	if ( !bValid )
	{
		FrameSize = DesignSize;
	}
	Scale = bValid ? std::min(FrameSize.X / DesignSize.X, FrameSize.Y / DesignSize.Y) : 1.0f;
}

float ScreenLayout::GetScale() const
{
	return Scale;
}

const Vec2f& ScreenLayout::GetDesignSize() const
{
	return DesignSize;
}

Vec2f ScreenLayout::GetVisibleSize() const
{
	return Vec2f(std::max(DesignSize.X, FrameSize.X / Scale), std::max(DesignSize.Y, FrameSize.Y / Scale));
}

Vec2f ScreenLayout::GetDesignOffset() const
{
	return (GetVisibleSize() - DesignSize) / 2.0f;
}

RectF ScreenLayout::GetVisibleRect() const
{
	return RectF(-GetDesignOffset(), GetVisibleSize());
}

Vec2f ScreenLayout::GetAnchor(const ScreenAnchor InAnchor) const
{
	const RectF Visible = GetVisibleRect();
	const Vec2f Center = Visible.GetCenter();
	switch ( InAnchor )
	{
		case ScreenAnchor::BottomLeft: return Vec2f(Visible.GetMinX(), Visible.GetMinY());
		case ScreenAnchor::Bottom: return Vec2f(Center.X, Visible.GetMinY());
		case ScreenAnchor::BottomRight: return Vec2f(Visible.GetMaxX(), Visible.GetMinY());
		case ScreenAnchor::Left: return Vec2f(Visible.GetMinX(), Center.Y);
		case ScreenAnchor::Right: return Vec2f(Visible.GetMaxX(), Center.Y);
		case ScreenAnchor::TopLeft: return Vec2f(Visible.GetMinX(), Visible.GetMaxY());
		case ScreenAnchor::Top: return Vec2f(Center.X, Visible.GetMaxY());
		case ScreenAnchor::TopRight: return Vec2f(Visible.GetMaxX(), Visible.GetMaxY());
		default: return Center;
	}
}

Vec2f ScreenLayout::Place(const ScreenAnchor InAnchor, const Vec2f& InOffset) const
{
	return GetAnchor(InAnchor) + InOffset;
}

} // namespace tanks::model

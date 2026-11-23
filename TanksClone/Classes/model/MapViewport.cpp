#include "model/MapViewport.h"

#include <algorithm>
#include <cmath>

namespace tanks::model
{

namespace
{
	float ClampAxis(const float InScroll, const float InContent, const float InView)
	{
		if ( InContent <= InView )
		{
			return (InContent - InView) / 2.0f;
		}
		return std::clamp(InScroll, 0.0f, InContent - InView);
	}

	float ThumbLength(const float InBarLength, const float InContent, const float InView)
	{
		if ( InContent <= InView || InContent <= 0.0f )
		{
			return InBarLength;
		}
		return std::max(std::min(InBarLength, 12.0f), InBarLength * InView / InContent);
	}

	// 0 at the start of the range, 1 at its end.
	float Fraction(const float InScroll, const float InContent, const float InView)
	{
		return InContent > InView ? InScroll / (InContent - InView) : 0.0f;
	}
}

void MapViewport::SetContentSize(const Vec2f& InSize)
{
	ContentSize = InSize;
	Clamp();
}

void MapViewport::SetViewSize(const Vec2f& InSize)
{
	ViewSize = InSize;
	Clamp();
}

void MapViewport::SetScroll(const Vec2f& InScroll)
{
	Scroll = InScroll;
	Clamp();
}

void MapViewport::ScrollBy(const Vec2f& InDelta)
{
	SetScroll(Scroll + InDelta);
}

void MapViewport::CenterOn(const Vec2f& InContentPoint)
{
	SetScroll(InContentPoint - ViewSize / 2.0f);
}

void MapViewport::Clamp()
{
	Scroll.X = ClampAxis(Scroll.X, ContentSize.X, ViewSize.X);
	Scroll.Y = ClampAxis(Scroll.Y, ContentSize.Y, ViewSize.Y);
}

bool MapViewport::CellAt(const Vec2f& InContentPoint, const float InTileSize, EditorCell& OutCell) const
{
	if ( InTileSize <= 0.0f || InContentPoint.X < 0.0f || InContentPoint.Y < 0.0f || InContentPoint.X >= ContentSize.X || InContentPoint.Y >= ContentSize.Y )
	{
		return false;
	}
	const int Rows = static_cast<int>(std::lround(ContentSize.Y / InTileSize));
	OutCell.Column = static_cast<int>(InContentPoint.X / InTileSize);
	OutCell.Row = Rows - 1 - static_cast<int>(InContentPoint.Y / InTileSize);
	return true;
}

Vec2f MapViewport::CellCenter(const EditorCell& InCell, const float InTileSize) const
{
	const int Rows = static_cast<int>(std::lround(ContentSize.Y / InTileSize));
	return Vec2f((InCell.Column + 0.5f) * InTileSize, (Rows - InCell.Row - 0.5f) * InTileSize);
}

void MapViewport::CenterOnCell(const EditorCell& InCell, const float InTileSize)
{
	CenterOn(CellCenter(InCell, InTileSize));
}

RectF MapViewport::GetHorizontalThumb(const RectF& InBar) const
{
	const float Length = ThumbLength(InBar.Size.X, ContentSize.X, ViewSize.X);
	const float Start = InBar.GetMinX() + (InBar.Size.X - Length) * Fraction(Scroll.X, ContentSize.X, ViewSize.X);
	return RectF(Start, InBar.GetMinY(), Length, InBar.Size.Y);
}

RectF MapViewport::GetVerticalThumb(const RectF& InBar) const
{
	const float Length = ThumbLength(InBar.Size.Y, ContentSize.Y, ViewSize.Y);
	const float Start = InBar.GetMinY() + (InBar.Size.Y - Length) * Fraction(Scroll.Y, ContentSize.Y, ViewSize.Y);
	return RectF(InBar.GetMinX(), Start, InBar.Size.X, Length);
}

void MapViewport::ScrollToHorizontalThumb(const RectF& InBar, const float InBarX)
{
	const float Length = ThumbLength(InBar.Size.X, ContentSize.X, ViewSize.X);
	const float Room = InBar.Size.X - Length;
	const float Share = Room > 0.0f ? std::clamp((InBarX - Length / 2.0f - InBar.GetMinX()) / Room, 0.0f, 1.0f) : 0.0f;
	SetScroll(Vec2f(Share * std::max(0.0f, ContentSize.X - ViewSize.X), Scroll.Y));
}

void MapViewport::ScrollToVerticalThumb(const RectF& InBar, const float InBarY)
{
	const float Length = ThumbLength(InBar.Size.Y, ContentSize.Y, ViewSize.Y);
	const float Room = InBar.Size.Y - Length;
	const float Share = Room > 0.0f ? std::clamp((InBarY - Length / 2.0f - InBar.GetMinY()) / Room, 0.0f, 1.0f) : 0.0f;
	SetScroll(Vec2f(Scroll.X, Share * std::max(0.0f, ContentSize.Y - ViewSize.Y)));
}

RectF MapViewport::GetMinimapContentRect(const RectF& InMinimap) const
{
	if ( ContentSize.X <= 0.0f || ContentSize.Y <= 0.0f )
	{
		return RectF(InMinimap.Origin, Vec2f());
	}
	const float Factor = std::min(InMinimap.Size.X / ContentSize.X, InMinimap.Size.Y / ContentSize.Y);
	return RectF::FromCenter(InMinimap.GetCenter(), ContentSize * Factor);
}

RectF MapViewport::GetMinimapViewRect(const RectF& InMinimap) const
{
	const RectF Content = GetMinimapContentRect(InMinimap);
	if ( ContentSize.X <= 0.0f || ContentSize.Y <= 0.0f )
	{
		return Content;
	}
	const float MinX = std::clamp(Scroll.X, 0.0f, ContentSize.X);
	const float MinY = std::clamp(Scroll.Y, 0.0f, ContentSize.Y);
	const float MaxX = std::clamp(Scroll.X + ViewSize.X, 0.0f, ContentSize.X);
	const float MaxY = std::clamp(Scroll.Y + ViewSize.Y, 0.0f, ContentSize.Y);
	const float Factor = Content.Size.X / ContentSize.X;
	return RectF(Content.GetMinX() + MinX * Factor, Content.GetMinY() + MinY * Factor, (MaxX - MinX) * Factor, (MaxY - MinY) * Factor);
}

Vec2f MapViewport::MinimapToContent(const RectF& InMinimap, const Vec2f& InPoint) const
{
	const RectF Content = GetMinimapContentRect(InMinimap);
	if ( Content.Size.X <= 0.0f )
	{
		return Vec2f();
	}
	const float Factor = ContentSize.X / Content.Size.X;
	return Vec2f(std::clamp((InPoint.X - Content.GetMinX()) * Factor, 0.0f, ContentSize.X), std::clamp((InPoint.Y - Content.GetMinY()) * Factor, 0.0f, ContentSize.Y));
}

} // namespace tanks::model

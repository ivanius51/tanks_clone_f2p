#pragma once

#include "model/EditorDocument.h"
#include "model/MathTypes.h"

namespace tanks::model
{
/**
 * The window onto a map of any size: a view rectangle that scrolls over the content. Content space has its origin at
 * the lower left, Y up, like the screen; row 0 of the map is the top row. A map smaller than the view is centred.
 * Also gives the scrollbar thumbs and the minimap mapping, so any map size is checked by unit tests.
 */
class MapViewport
{
public:
	void SetContentSize(const Vec2f& InSize);
	void SetViewSize(const Vec2f& InSize);
	const Vec2f& GetContentSize() const { return ContentSize; }
	const Vec2f& GetViewSize() const { return ViewSize; }

	Vec2f GetScroll() const { return Scroll; }
	void SetScroll(const Vec2f& InScroll);
	void ScrollBy(const Vec2f& InDelta);
	void CenterOn(const Vec2f& InContentPoint);

	bool CanScrollX() const { return ContentSize.X > ViewSize.X; }
	bool CanScrollY() const { return ContentSize.Y > ViewSize.Y; }

	Vec2f ViewToContent(const Vec2f& InViewPoint) const { return InViewPoint + Scroll; }
	Vec2f ContentToView(const Vec2f& InContentPoint) const { return InContentPoint - Scroll; }

	bool CellAt(const Vec2f& InContentPoint, float InTileSize, EditorCell& OutCell) const;
	Vec2f CellCenter(const EditorCell& InCell, float InTileSize) const;
	void CenterOnCell(const EditorCell& InCell, float InTileSize);

	RectF GetHorizontalThumb(const RectF& InBar) const;
	RectF GetVerticalThumb(const RectF& InBar) const;
	void ScrollToHorizontalThumb(const RectF& InBar, float InBarX);
	void ScrollToVerticalThumb(const RectF& InBar, float InBarY);

	RectF GetMinimapViewRect(const RectF& InMinimap) const;
	Vec2f MinimapToContent(const RectF& InMinimap, const Vec2f& InPoint) const;
	RectF GetMinimapContentRect(const RectF& InMinimap) const;

private:
	void Clamp();

	Vec2f ContentSize;
	Vec2f ViewSize;
	Vec2f Scroll;
};
} // namespace tanks::model

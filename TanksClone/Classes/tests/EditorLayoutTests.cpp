#include "tests/EditorLayoutTests.h"

#include <vector>

#include "model/EditorLayout.h"
#include "model/MapViewport.h"

namespace tanks
{
namespace tests
{

namespace
{
	bool IsInside(const model::RectF& InOuter, const model::RectF& InInner)
	{
		return InInner.GetMinX() >= InOuter.GetMinX() - 0.001f && InInner.GetMinY() >= InOuter.GetMinY() - 0.001f
			&& InInner.GetMaxX() <= InOuter.GetMaxX() + 0.001f && InInner.GetMaxY() <= InOuter.GetMaxY() + 0.001f;
	}

	bool HasOverlap(const std::vector<model::RectF>& InRects)
	{
		for ( std::size_t First = 0; First < InRects.size(); ++First )
		{
			for ( std::size_t Second = First + 1; Second < InRects.size(); ++Second )
			{
				if ( InRects[First].Intersects(InRects[Second]) )
				{
					return true;
				}
			}
		}
		return false;
	}
}

void AddEditorLayoutTests(TestSuite& InSuite)
{
	using model::EditorDockState;
	using model::EditorLayout;
	using model::EditorLayoutResult;
	using model::RectF;
	using model::Vec2f;

	InSuite.Add("EditorLayout: docks, map and bars never overlap at any screen size or collapse state", [](TestContext& InContext)
	{
		const std::vector<RectF> Screens = { RectF(0, 0, 1024, 576), RectF(0, 0, 1920, 1080), RectF(-80, 0, 1184, 864), RectF(0, 0, 640, 360), RectF(0, 0, 1280, 800) };
		for ( const RectF& Bounds : Screens )
		{
			for ( int Mask = 0; Mask < 8; ++Mask )
			{
				EditorDockState State;
				State.bLeftCollapsed = (Mask & 1) != 0;
				State.bRightCollapsed = (Mask & 2) != 0;
				State.bBottomCollapsed = (Mask & 4) != 0;
				const EditorLayoutResult Layout = EditorLayout().Compute(Bounds, State);
				const std::string Where = "width " + std::to_string(static_cast<int>(Bounds.Size.X)) + ", mask " + std::to_string(Mask);
				const std::vector<RectF> All = { Layout.LeftDock, Layout.RightDock, Layout.BottomDock, Layout.MapView, Layout.VerticalBar, Layout.HorizontalBar };
				InContext.Expect(!HasOverlap(All), "no overlaps, " + Where);
				for ( const RectF& Rect : All )
				{
					InContext.Expect(IsInside(Bounds, Rect), "inside the screen, " + Where);
				}
				InContext.Expect(IsInside(Layout.MapView, Layout.Minimap), "minimap over the map, " + Where);
				InContext.Expect(Layout.MapView.Size.X > 0.0f && Layout.MapView.Size.Y > 0.0f, "the map keeps room, " + Where);
			}
		}
	});

	InSuite.Add("EditorLayout: a collapsed dock keeps a strip and a narrow window collapses the side docks", [](TestContext& InContext)
	{
		const model::EditorLayoutSettings Settings;
		const RectF Wide(0, 0, 1920, 1080);
		EditorDockState State;
		const float Open = EditorLayout().Compute(Wide, State).LeftDock.Size.X;
		State.bLeftCollapsed = true;
		const EditorLayoutResult Closed = EditorLayout().Compute(Wide, State);
		InContext.ExpectNear(Closed.LeftDock.Size.X, Settings.CollapsedSize, 0.01f, "a collapsed dock is its handle strip");
		InContext.Expect(Open > Closed.LeftDock.Size.X, "collapsing frees room");
		InContext.Expect(Closed.MapView.Size.X > EditorLayout().Compute(Wide, EditorDockState()).MapView.Size.X, "the map grows");
		const EditorLayoutResult Narrow = EditorLayout().Compute(RectF(0, 0, 700, 400), EditorDockState());
		InContext.Expect(Narrow.bLeftCollapsed && Narrow.bRightCollapsed, "both side docks collapse on a narrow window");
		InContext.Expect(!EditorLayout().Compute(Wide, EditorDockState()).bRightCollapsed, "and stay open on a wide one");
		EditorDockState Kept;
		Kept.bAutoCollapse = false;
		const EditorLayoutResult Open700 = EditorLayout().Compute(RectF(0, 0, 700, 400), Kept);
		InContext.Expect(!Open700.bLeftCollapsed && !Open700.bRightCollapsed, "with auto collapse off a narrow window keeps the docks open");
		Kept.bLeftCollapsed = true;
		InContext.Expect(EditorLayout().Compute(RectF(0, 0, 700, 400), Kept).bLeftCollapsed, "and an explicit collapse still wins");
	});

	InSuite.Add("MapViewport: scroll is clamped and a small map is centred", [](TestContext& InContext)
	{
		model::MapViewport View;
		View.SetViewSize(Vec2f(400, 300));
		View.SetContentSize(Vec2f(1000, 2000));
		View.SetScroll(Vec2f(-50, 99999));
		InContext.Expect(View.GetScroll() == Vec2f(0, 1700), "clamped to the content");
		View.ScrollBy(Vec2f(5000, -10));
		InContext.Expect(View.GetScroll() == Vec2f(600, 1690), "scroll by");
		View.SetContentSize(Vec2f(200, 100));
		InContext.Expect(View.GetScroll() == Vec2f(-100, -100), "a small map is centred in the view");
		InContext.Expect(!View.CanScrollX() && !View.CanScrollY(), "nothing to scroll");
		View.CenterOn(Vec2f(1000, 1000));
		InContext.Expect(View.GetScroll() == Vec2f(-100, -100), "centring a small map does not move it");
	});

	InSuite.Add("MapViewport: cells map to content points and back, row 0 is the top", [](TestContext& InContext)
	{
		model::MapViewport View;
		View.SetViewSize(Vec2f(200, 200));
		View.SetContentSize(Vec2f(64 * 10, 64 * 5));
		model::EditorCell Cell;
		InContext.Expect(View.CellAt(Vec2f(1, 319), 64, Cell) && Cell.Row == 0 && Cell.Column == 0, "top-left cell");
		InContext.Expect(View.CellAt(Vec2f(639, 1), 64, Cell) && Cell.Row == 4 && Cell.Column == 9, "bottom-right cell");
		InContext.Expect(!View.CellAt(Vec2f(640, 10), 64, Cell) && !View.CellAt(Vec2f(-1, 10), 64, Cell), "outside the map");
		const Vec2f Centre = View.CellCenter(model::EditorCell{ 0, 2 }, 64);
		InContext.Expect(Centre == Vec2f(160, 288), "cell centre");
		View.CenterOnCell(model::EditorCell{ 4, 9 }, 64);
		InContext.Expect(View.GetScroll() == Vec2f(440, 0), "centring at the corner clamps");
	});

	InSuite.Add("MapViewport: scrollbar thumbs and the minimap follow the scroll", [](TestContext& InContext)
	{
		model::MapViewport View;
		View.SetViewSize(Vec2f(400, 400));
		View.SetContentSize(Vec2f(1600, 800));
		const RectF HorizontalBar(0, 0, 400, 14);
		const RectF VerticalBar(400, 14, 14, 400);
		RectF Thumb = View.GetHorizontalThumb(HorizontalBar);
		InContext.ExpectNear(Thumb.Size.X, 100.0f, 0.01f, "thumb length is the visible share");
		InContext.ExpectNear(Thumb.GetMinX(), 0.0f, 0.01f, "thumb at the start");
		View.SetScroll(Vec2f(1200, 400));
		Thumb = View.GetHorizontalThumb(HorizontalBar);
		InContext.ExpectNear(Thumb.GetMaxX(), 400.0f, 0.01f, "thumb at the end");
		InContext.ExpectNear(View.GetVerticalThumb(VerticalBar).GetMaxY(), 414.0f, 0.01f, "scrolled up the most: thumb at the top");
		View.ScrollToHorizontalThumb(HorizontalBar, 50.0f);
		InContext.ExpectNear(View.GetScroll().X, 0.0f, 0.01f, "dragging the thumb to the start");
		View.ScrollToHorizontalThumb(HorizontalBar, 200.0f);
		InContext.ExpectNear(View.GetScroll().X, 600.0f, 0.01f, "dragging the thumb to the middle");
		View.ScrollToVerticalThumb(VerticalBar, 14.0f + 400.0f);
		InContext.ExpectNear(View.GetScroll().Y, 400.0f, 0.01f, "thumb at the top is the most scrolled up");

		const RectF Minimap(0, 0, 100, 100);
		View.SetScroll(Vec2f(0, 0));
		const RectF Shown = View.GetMinimapViewRect(Minimap);
		InContext.ExpectNear(Shown.Size.X, 25.0f, 0.01f, "minimap view width");
		InContext.ExpectNear(Shown.Size.Y, 25.0f, 0.01f, "minimap view height");
		const Vec2f Point = View.MinimapToContent(Minimap, Minimap.GetCenter());
		InContext.Expect(Point == Vec2f(800, 400), "the minimap centre is the content centre");
	});
}

} // namespace tests
} // namespace tanks

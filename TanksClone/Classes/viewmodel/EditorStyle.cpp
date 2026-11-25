#include "viewmodel/EditorStyle.h"

#include "model/ConfigFields.h"

namespace game::viewmodel
{

EditorStyle EditorStyle::FromConfig(const model::IConfigNode* const InNode, std::vector<std::string>& OutMissing)
{
	EditorStyle Result;
	const model::ConfigFields Root(InNode, OutMissing);

	const model::ConfigFields Layout = Root.Child("layout");
	model::EditorLayoutSettings& Sizes = Result.Layout;
	Layout.Read("padding", Sizes.Padding);
	Layout.Read("gap", Sizes.Gap);
	Layout.Read("side_width_share", Sizes.SideWidthShare);
	Layout.Read("min_side_width", Sizes.MinSideWidth);
	Layout.Read("max_side_width", Sizes.MaxSideWidth);
	Layout.Read("bottom_height_share", Sizes.BottomHeightShare);
	Layout.Read("min_bottom_height", Sizes.MinBottomHeight);
	Layout.Read("max_bottom_height", Sizes.MaxBottomHeight);
	Layout.Read("collapsed_size", Sizes.CollapsedSize);
	Layout.Read("scrollbar_size", Sizes.ScrollbarSize);
	Layout.Read("minimap_size", Sizes.MinimapSize);
	Layout.Read("collapse_right_below_width", Sizes.CollapseRightBelowWidth);
	Layout.Read("collapse_left_below_width", Sizes.CollapseLeftBelowWidth);

	Root.Read("icon_fill", Result.IconFill);
	Root.Read("enemy_label_font_size", Result.EnemyLabelFontSize);
	Root.Read("message_bottom", Result.MessageBottom);
	Root.Read("button_height", Result.ButtonHeight);

	const model::ConfigFields Limits = Root.Child("limits");
	Limits.Read("min_enemies", Result.MinEnemies);
	Limits.Read("max_enemies", Result.MaxEnemies);
	Limits.Read("max_alive", Result.MaxAlive);
	Limits.Read("library_rows", Result.LibraryRows);
	Root.Read("dev_resources_root", Result.DevResourcesRoot);
	const model::ConfigFields Zoom = Root.Child("zoom");
	Zoom.Read("min", Result.MinZoom);
	Zoom.Read("max", Result.MaxZoom);
	Zoom.Read("step", Result.ZoomStep);

	const model::ConfigFields Colors = Root.Child("colors");
	Colors.Read("panel", Result.PanelColor);
	Colors.Read("panel_border", Result.PanelBorder);
	Colors.Read("erase", Result.EraseColor);
	return Result;
}

} // namespace game::viewmodel

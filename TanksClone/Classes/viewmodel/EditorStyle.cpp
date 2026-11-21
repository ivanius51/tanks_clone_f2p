#include "viewmodel/EditorStyle.h"

#include "model/ConfigFields.h"

namespace tanks::viewmodel
{

EditorStyle EditorStyle::FromConfig(const model::IConfigNode* const InNode, std::vector<std::string>& OutMissing)
{
	EditorStyle Result;
	const model::ConfigFields Root(InNode, OutMissing);

	const model::ConfigFields Layout = Root.Child("layout");
	model::EditorLayoutSettings& Sizes = Result.Layout;
	Layout.Read("padding", Sizes.Padding);
	Layout.Read("gap", Sizes.Gap);
	Layout.Read("max_tab_height", Sizes.MaxTabHeight);
	Layout.Read("max_item_size", Sizes.MaxItemSize);
	Layout.Read("min_item_size", Sizes.MinItemSize);
	Layout.Read("max_action_panel_width", Sizes.MaxActionPanelWidth);
	Layout.Read("action_panel_share", Sizes.ActionPanelShare);
	Layout.Read("action_columns", Sizes.ActionColumns);

	Root.Read("icon_fill", Result.IconFill);
	Root.Read("enemy_label_font_size", Result.EnemyLabelFontSize);
	Root.Read("message_bottom", Result.MessageBottom);

	const model::ConfigFields Limits = Root.Child("limits");
	Limits.Read("min_enemies", Result.MinEnemies);
	Limits.Read("max_enemies", Result.MaxEnemies);
	Limits.Read("max_alive", Result.MaxAlive);

	const model::ConfigFields Colors = Root.Child("colors");
	Colors.Read("toolbar", Result.ToolbarColor);
	Colors.Read("panel", Result.PanelColor);
	Colors.Read("panel_border", Result.PanelBorder);
	Colors.Read("erase", Result.EraseColor);
	return Result;
}

} // namespace tanks::viewmodel

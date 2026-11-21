#include "viewmodel/MenuLayout.h"

#include "model/ConfigFields.h"

namespace tanks::viewmodel
{

namespace
{
	void ReadMain(const model::ConfigFields& InFields, MainMenuLayout& OutLayout)
	{
		InFields.Read("title_font_size", OutLayout.TitleFontSize);
		InFields.Read("info_font_size", OutLayout.InfoFontSize);
		InFields.Read("title_top", OutLayout.TitleTop);
		InFields.Read("info_top", OutLayout.InfoTop);
		InFields.Read("button_width", OutLayout.ButtonWidth);
		InFields.Read("button_height", OutLayout.ButtonHeight);
		InFields.Read("button_spacing", OutLayout.ButtonSpacing);
		InFields.Read("list_offset_y", OutLayout.ListOffsetY);
	}

	void ReadSettings(const model::ConfigFields& InFields, SettingsLayout& OutLayout)
	{
		InFields.Read("title_font_size", OutLayout.TitleFontSize);
		InFields.Read("button_width", OutLayout.ButtonWidth);
		InFields.Read("button_height", OutLayout.ButtonHeight);
		InFields.Read("button_spacing", OutLayout.ButtonSpacing);
		InFields.Read("list_center_offset", OutLayout.ListCenterOffset);
		InFields.Read("title_top_offset", OutLayout.TitleTopOffset);
		InFields.Read("panel_padding", OutLayout.PanelPadding);
		InFields.Read("title_gap", OutLayout.TitleGap);
		InFields.Read("volume_step", OutLayout.VolumeStep);
	}

	void ReadUpgrades(const model::ConfigFields& InFields, UpgradesLayout& OutLayout)
	{
		InFields.Read("title_font_size", OutLayout.TitleFontSize);
		InFields.Read("name_font_size", OutLayout.NameFontSize);
		InFields.Read("info_font_size", OutLayout.InfoFontSize);
		InFields.Read("title_top", OutLayout.TitleTop);
		InFields.Read("coins_top", OutLayout.CoinsTop);
		InFields.Read("first_row_top", OutLayout.FirstRowTop);
		InFields.Read("row_step", OutLayout.RowStep);
		InFields.Read("name_x", OutLayout.NameX);
		InFields.Read("name_offset_y", OutLayout.NameOffsetY);
		InFields.Read("info_offset_y", OutLayout.InfoOffsetY);
		InFields.Read("pips_x", OutLayout.PipsX);
		InFields.Read("pip_size", OutLayout.PipSize);
		InFields.Read("pip_step", OutLayout.PipStep);
		InFields.Read("pip_border", OutLayout.PipBorder);
		InFields.Read("buy_x", OutLayout.BuyX);
		InFields.Read("buy_width", OutLayout.BuyWidth);
		InFields.Read("buy_height", OutLayout.BuyHeight);
		InFields.Read("back_bottom", OutLayout.BackBottom);
		InFields.Read("pip_edge", OutLayout.PipEdge);
		InFields.Read("pip_full", OutLayout.PipFull);
		InFields.Read("pip_empty", OutLayout.PipEmpty);
	}

	void ReadLoading(const model::ConfigFields& InFields, LoadingLayout& OutLayout)
	{
		InFields.Read("title_font_size", OutLayout.TitleFontSize);
		InFields.Read("status_font_size", OutLayout.StatusFontSize);
		InFields.Read("title_offset_y", OutLayout.TitleOffsetY);
		InFields.Read("status_offset_y", OutLayout.StatusOffsetY);
		InFields.Read("bar_offset_y", OutLayout.BarOffsetY);
		InFields.Read("bar_width", OutLayout.BarWidth);
		InFields.Read("bar_height", OutLayout.BarHeight);
		InFields.Read("bar_border", OutLayout.BarBorder);
		InFields.Read("min_show_time", OutLayout.MinShowTime);
		InFields.Read("start_delay", OutLayout.StartDelay);
	}
}

MenuLayout MenuLayout::FromConfig(const model::IConfigNode* const InNode, std::vector<std::string>& OutMissing)
{
	MenuLayout Result;
	const model::ConfigFields Root(InNode, OutMissing);
	ReadMain(Root.Child("main"), Result.Main);
	ReadSettings(Root.Child("settings"), Result.Settings);
	ReadUpgrades(Root.Child("upgrades"), Result.Upgrades);
	ReadLoading(Root.Child("loading"), Result.Loading);
	return Result;
}

} // namespace tanks::viewmodel

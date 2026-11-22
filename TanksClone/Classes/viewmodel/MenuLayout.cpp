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
		InFields.Read("device_name_chars", OutLayout.DeviceNameChars);
	}

	void ReadUpgrades(const model::ConfigFields& InFields, UpgradesLayout& OutLayout)
	{
		InFields.Read("title_font_size", OutLayout.TitleFontSize);
		InFields.Read("name_font_size", OutLayout.NameFontSize);
		InFields.Read("info_font_size", OutLayout.InfoFontSize);
		InFields.Read("node_font_size", OutLayout.NodeFontSize);
		InFields.Read("price_font_size", OutLayout.PriceFontSize);
		InFields.Read("panel_x", OutLayout.PanelX);
		InFields.Read("panel_width", OutLayout.PanelWidth);
		InFields.Read("title_top", OutLayout.TitleTop);
		InFields.Read("coins_top", OutLayout.CoinsTop);
		InFields.Read("name_top", OutLayout.NameTop);
		InFields.Read("info_top", OutLayout.InfoTop);
		InFields.Read("level_top", OutLayout.LevelTop);
		InFields.Read("need_top", OutLayout.NeedTop);
		InFields.Read("buy_bottom", OutLayout.BuyBottom);
		InFields.Read("back_bottom", OutLayout.BackBottom);
		InFields.Read("button_width", OutLayout.ButtonWidth);
		InFields.Read("button_height", OutLayout.ButtonHeight);
		InFields.Read("node_size", OutLayout.NodeSize);
		InFields.Read("icon_size", OutLayout.IconSize);
		InFields.Read("name_gap", OutLayout.NameGap);
		InFields.Read("badge_size", OutLayout.BadgeSize);
		InFields.Read("ring_width", OutLayout.RingWidth);
		InFields.Read("ring_gap", OutLayout.RingGap);
		InFields.Read("ring_segment_gap", OutLayout.RingSegmentGap);
		InFields.Read("link_width", OutLayout.LinkWidth);
		InFields.Read("focus_width", OutLayout.FocusWidth);
		InFields.Read("focus_gap", OutLayout.FocusGap);
		InFields.Read("grid_step", OutLayout.GridStep);
		InFields.Read("grid_major_every", OutLayout.GridMajorEvery);
		InFields.Read("background_color", OutLayout.BackgroundColor);
		InFields.Read("grid_color", OutLayout.GridColor);
		InFields.Read("grid_major_color", OutLayout.GridMajorColor);
		InFields.Read("panel_color", OutLayout.PanelColor);
		InFields.Read("edge_color", OutLayout.EdgeColor);
		InFields.Read("box_color", OutLayout.BoxColor);
		InFields.Read("node_fill", OutLayout.NodeFill);
		InFields.Read("node_ready", OutLayout.NodeReady);
		InFields.Read("node_done", OutLayout.NodeDone);
		InFields.Read("node_locked", OutLayout.NodeLocked);
		InFields.Read("link_on", OutLayout.LinkOn);
		InFields.Read("link_off", OutLayout.LinkOff);
		InFields.Read("ring_empty", OutLayout.RingEmpty);
		InFields.Read("ring_full", OutLayout.RingFull);
		InFields.Read("focus_color", OutLayout.FocusColor);
		InFields.Read("badge_color", OutLayout.BadgeColor);
		InFields.Read("turret_point", OutLayout.TurretPoint);
		InFields.Read("box_origin", OutLayout.BoxOrigin);
		InFields.Read("box_size", OutLayout.BoxSize);
		InFields.Read("selected_weapon", OutLayout.SelectedWeapon);
		InFields.Read("weapons_open_seconds", OutLayout.WeaponsOpenSeconds);
		InFields.Read("weapons_stagger_seconds", OutLayout.WeaponsStaggerSeconds);
		const model::IConfigNode* const Root = InFields.GetNode();
		const model::IConfigNode* const Views = Root ? Root->FindChild("views") : nullptr;
		const model::IConfigNode* const Weapons = Root ? Root->FindChild("weapons") : nullptr;
		const model::IConfigNode* const Branches = Root ? Root->FindChild("branches") : nullptr;
		if ( !Views || !Weapons || !Branches )
		{
			std::vector<std::string> ReportMissing;
			InFields.Read("views", ReportMissing);
			InFields.Read("weapons", ReportMissing);
			InFields.Read("branches", ReportMissing);
			return;
		}
		OutLayout.Views.clear();
		for ( std::size_t Index = 0; Index < Views->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *Views->GetItem(Index);
			UpgradeViewLayout View;
			if ( Item.TryRead("frame", View.Frame) && Item.TryRead("center", View.Center) && Item.TryRead("width", View.Width) )
			{
				OutLayout.Views.push_back(View);
			}
		}
		OutLayout.Weapons.clear();
		for ( std::size_t Index = 0; Index < Weapons->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *Weapons->GetItem(Index);
			UpgradeWeaponTab Tab;
			if ( Item.TryRead("branch", Tab.Branch) && Item.TryRead("name", Tab.NameKey) && Item.TryRead("icon", Tab.Icon) )
			{
				OutLayout.Weapons.push_back(Tab);
			}
		}
		OutLayout.Branches.clear();
		for ( std::size_t Index = 0; Index < Branches->GetItemCount(); ++Index )
		{
			model::UpgradeBranch Branch;
			if ( model::UpgradeTree::ReadBranch(*Branches->GetItem(Index), Branch) )
			{
				OutLayout.Branches.push_back(Branch);
			}
		}
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

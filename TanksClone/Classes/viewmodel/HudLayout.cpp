#include "viewmodel/HudLayout.h"

#include "model/ConfigFields.h"

namespace game::viewmodel
{

namespace
{
	void ReadColors(const model::ConfigFields& InFields, HudColors& OutColors)
	{
		InFields.Read("panel", OutColors.PanelFill);
		InFields.Read("panel_border", OutColors.PanelBorder);
		InFields.Read("health", OutColors.HealthColor);
		InFields.Read("health_low", OutColors.HealthLowColor);
		InFields.Read("ammo", OutColors.AmmoColor);
		InFields.Read("ammo_empty", OutColors.AmmoEmptyColor);
		InFields.Read("slot", OutColors.SlotFill);
		InFields.Read("slot_border", OutColors.SlotBorder);
		InFields.Read("slot_ready_border", OutColors.SlotReadyBorder);
		InFields.Read("cooldown_shade", OutColors.CooldownShade);
	}

	void ReadTop(const model::ConfigFields& InFields, HudLayout& OutLayout)
	{
		InFields.Read("anchor", OutLayout.TopAnchor);
		InFields.Read("padding", OutLayout.TopPadding);
		InFields.Read("font_size", OutLayout.TopFontSize);
		InFields.Read("small_font_size", OutLayout.TopSmallFontSize);
		InFields.Read("wave_x", OutLayout.WaveX);
		InFields.Read("enemies_x", OutLayout.EnemiesX);
		InFields.Read("enemies_bar_x", OutLayout.EnemiesBarX);
		InFields.Read("band_border", OutLayout.BandBorder);
	}

	void ReadPanel(const model::ConfigFields& InFields, HudLayout& OutLayout)
	{
		InFields.Read("anchor", OutLayout.PanelAnchor);
		InFields.Read("portrait_x", OutLayout.PortraitX);
		InFields.Read("portrait_size", OutLayout.PortraitSize);
		InFields.Read("portrait_inset", OutLayout.PortraitInset);
		InFields.Read("frame_border", OutLayout.FrameBorder);
		InFields.Read("bars_x", OutLayout.BarsX);
		InFields.Read("health_y", OutLayout.HealthY);
		InFields.Read("ammo_y", OutLayout.AmmoY);
		InFields.Read("reload_y", OutLayout.ReloadY);
		InFields.Read("stats_y", OutLayout.StatsY);
		InFields.Read("stats_font_size", OutLayout.StatsFontSize);
		InFields.Read("health_low_percent", OutLayout.HealthLowPercent);
	}

	void ReadSlots(const model::ConfigFields& InFields, HudLayout& OutLayout)
	{
		InFields.Read("x", OutLayout.SlotsX);
		InFields.Read("step", OutLayout.SlotStep);
		InFields.Read("size", OutLayout.SlotSize);
		InFields.Read("icon_size", OutLayout.SlotIconSize);
		InFields.Read("icon_offset_y", OutLayout.SlotIconOffsetY);
		InFields.Read("max", OutLayout.MaxSlots);
		InFields.Read("slots_offset_y", OutLayout.SlotsOffsetY);
		InFields.Read("key_inset_y", OutLayout.KeyInsetY);
		InFields.Read("timer_font_size", OutLayout.TimerFontSize);
		InFields.Read("timer_min_seconds", OutLayout.TimerMinSeconds);
	}

	void ReadBanner(const model::ConfigFields& InFields, HudLayout& OutLayout)
	{
		InFields.Read("font_size", OutLayout.BannerFontSize);
		InFields.Read("fade_in", OutLayout.BannerFadeIn);
		InFields.Read("hold", OutLayout.BannerHold);
		InFields.Read("fade_out", OutLayout.BannerFadeOut);
	}
}

HudLayout HudLayout::FromConfig(const model::IConfigNode* const InNode, std::vector<std::string>& OutMissing)
{
	HudLayout Result;
	const model::ConfigFields Root(InNode, OutMissing);
	ReadColors(Root.Child("colors"), Result.Colors);
	ReadTop(Root.Child("top"), Result);
	ReadPanel(Root.Child("panel"), Result);
	ReadSlots(Root.Child("slots"), Result);
	ReadBanner(Root.Child("banner"), Result);
	return Result;
}

} // namespace game::viewmodel

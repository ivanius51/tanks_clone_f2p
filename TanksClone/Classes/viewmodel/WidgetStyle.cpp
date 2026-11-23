#include "viewmodel/WidgetStyle.h"

#include "model/ConfigFields.h"

namespace tanks::viewmodel
{

const model::Color* WidgetColors::Find(const std::string_view InName) const
{
	struct Named
	{
		std::string_view Name;
		const model::Color WidgetColors::* Member;
	};
	static constexpr Named Table[] = {
		{ "text", &WidgetColors::TextNormal }, { "text_dim", &WidgetColors::TextDim }, { "accent", &WidgetColors::Accent },
		{ "good", &WidgetColors::Good }, { "bad", &WidgetColors::Bad }, { "panel", &WidgetColors::Panel },
		{ "panel_border", &WidgetColors::PanelBorder }, { "button", &WidgetColors::ButtonColor },
		{ "button_pressed", &WidgetColors::ButtonPressed }, { "button_disabled", &WidgetColors::ButtonDisabled },
		{ "button_border", &WidgetColors::ButtonBorder }, { "selection", &WidgetColors::Selection },
		{ "screen_background", &WidgetColors::ScreenBackground }, { "grid_line", &WidgetColors::GridLine },
		{ "dim", &WidgetColors::Dim }, { "outline", &WidgetColors::Outline } };
	for ( const Named& Current : Table )
	{
		if ( Current.Name == InName )
		{
			return &(this->*Current.Member);
		}
	}
	return nullptr;
}

WidgetStyle WidgetStyle::FromConfig(const model::IConfigNode* const InNode, std::vector<std::string>& OutMissing)
{
	WidgetStyle Result;
	const model::ConfigFields Root(InNode, OutMissing);
	if ( InNode && InNode->Has("text_scales") )
	{
		Result.TextScales = model::TextScales::FromConfig(*InNode);
	}
	else
	{
		OutMissing.emplace_back("text_scales");
	}
	Root.Read("system_font", Result.SystemFont);
	Root.Read("decorative_font_scale", Result.DecorativeFontScale);
	Root.Read("label_outline", Result.LabelOutline);

	const model::ConfigFields Colors = Root.Child("colors");
	WidgetColors& Palette = Result.Colors;
	Colors.Read("text", Palette.TextNormal);
	Colors.Read("text_dim", Palette.TextDim);
	Colors.Read("accent", Palette.Accent);
	Colors.Read("good", Palette.Good);
	Colors.Read("bad", Palette.Bad);
	Colors.Read("panel", Palette.Panel);
	Colors.Read("panel_border", Palette.PanelBorder);
	Colors.Read("button", Palette.ButtonColor);
	Colors.Read("button_pressed", Palette.ButtonPressed);
	Colors.Read("button_disabled", Palette.ButtonDisabled);
	Colors.Read("button_border", Palette.ButtonBorder);
	Colors.Read("selection", Palette.Selection);
	Colors.Read("screen_background", Palette.ScreenBackground);
	Colors.Read("grid_line", Palette.GridLine);
	Colors.Read("dim", Palette.Dim);
	Colors.Read("outline", Palette.Outline);

	const model::ConfigFields Button = Root.Child("button");
	Button.Read("height", Result.ButtonHeight);
	Button.Read("border", Result.BorderWidth);

	const model::ConfigFields Overlay = Root.Child("overlay");
	Overlay.Read("width", Result.OverlayWidth);
	Overlay.Read("padding", Result.OverlayPadding);
	Overlay.Read("button_spacing", Result.OverlayButtonSpacing);
	Overlay.Read("title_font_size", Result.OverlayTitleFontSize);
	Overlay.Read("message_font_size", Result.OverlayMessageFontSize);
	Overlay.Read("message_gap", Result.OverlayMessageGap);
	Overlay.Read("buttons_gap", Result.OverlayButtonsGap);

	const model::ConfigFields Toast = Root.Child("toast");
	Toast.Read("font_size", Result.ToastFontSize);
	Toast.Read("show_time", Result.ToastShowTime);
	Toast.Read("fade_time", Result.ToastFadeTime);
	Root.Read("scene_fade_time", Result.SceneFadeTime);
	const model::ConfigFields Hints = Root.Child("hints");
	Hints.Read("font_size", Result.HintsFontSize);
	Hints.Read("margin", Result.HintsMargin);
	return Result;
}

} // namespace tanks::viewmodel

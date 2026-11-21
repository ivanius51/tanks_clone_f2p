#include "viewmodel/WidgetStyle.h"

#include "model/ConfigFields.h"

namespace tanks::viewmodel
{

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
	Button.Read("font_size", Result.ButtonFontSize);

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
	return Result;
}

} // namespace tanks::viewmodel

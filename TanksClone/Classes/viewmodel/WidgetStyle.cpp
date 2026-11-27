#include "viewmodel/WidgetStyle.h"

#include "model/ConfigFields.h"
#include "model/TextSize.h"
#include "templates/TContainers.h"

namespace game::viewmodel
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

namespace
{
	bool ReadColorValue(const model::IConfigNode& InNode, const WidgetColors& InPalette, model::Color& OutColor)
	{
		if ( InNode.GetItemCount() > 0 )
		{
			return InNode.ReadSelf(OutColor);
		}
		std::string Name;
		const model::Color* const Found = InNode.ReadSelf(Name) ? InPalette.Find(Name) : nullptr;
		if ( Found )
		{
			OutColor = *Found;
		}
		return Found != nullptr;
	}

	// A flat color (shared name or [r, g, b, a]) or { "direction": ..., "stops": [color, ...] } under InKey.
	bool ReadFill(const model::IConfigNode& InParent, const std::string_view InKey, const WidgetColors& InPalette, model::Gradient& OutFill)
	{
		const model::IConfigNode* const Node = InParent.FindChild(InKey);
		if ( !Node )
		{
			return false;
		}
		if ( const model::IConfigNode* const Stops = Node->FindChild("stops") )
		{
			model::Gradient Result;
			Node->TryRead("direction", Result.Direction);
			for ( std::size_t Index = 0; Index < Stops->GetItemCount(); ++Index )
			{
				model::Color Stop;
				if ( ReadColorValue(*Stops->GetItem(Index), InPalette, Stop) )
				{
					Result.AddStop(Stop);
				}
			}
			OutFill = Result;
			return Result.IsSet();
		}
		model::Color Flat;
		if ( !ReadColorValue(*Node, InPalette, Flat) )
		{
			return false;
		}
		OutFill = model::FlatGradient(Flat);
		return true;
	}

	constexpr float PressedShade = 0.6f;

	ButtonStyle ReadButtonStyle(const model::IConfigNode& InNode, const WidgetColors& InPalette, const ButtonStyle& InBase)
	{
		ButtonStyle Result = InBase;
		const bool bFill = ReadFill(InNode, "fill", InPalette, Result.Fill);
		if ( !ReadFill(InNode, "pressed", InPalette, Result.Pressed) && bFill )
		{
			Result.Pressed = Result.Fill.Scaled(PressedShade);
		}
		ReadFill(InNode, "disabled", InPalette, Result.Disabled);
		if ( const model::IConfigNode* const Border = InNode.FindChild("border") )
		{
			ReadColorValue(*Border, InPalette, Result.Border);
		}
		if ( const model::IConfigNode* const Ink = InNode.FindChild("text") )
		{
			ReadColorValue(*Ink, InPalette, Result.Text);
		}
		InNode.TryRead("radius", Result.Radius);
		return Result;
	}
}

const ButtonStyle& WidgetStyle::FindButtonStyle(const std::string_view InId) const
{
	for ( const auto& Entry : ButtonStyles )
	{
		if ( Entry.first == InId )
		{
			return Entry.second;
		}
	}
	return DefaultButton;
}

TArray<std::string> WidgetStyle::GetButtonStyleIds() const
{
	TArray<std::string> Ids;
	Ids.PushBack("default");
	for ( const auto& Entry : ButtonStyles )
	{
		Ids.PushBack(Entry.first);
	}
	return Ids;
}

WidgetStyle WidgetStyle::FromConfig(const model::IConfigNode* const InNode, TArray<std::string>& OutMissing)
{
	WidgetStyle Result;
	const model::ConfigFields Root(InNode, OutMissing);
	if ( InNode && InNode->Has("text_scales") )
	{
		Result.TextScales = model::TextScales::FromConfig(*InNode);
	}
	else
	{
		OutMissing.EmplaceBack("text_scales");
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

	Result.DefaultButton.Fill = model::FlatGradient(Palette.ButtonColor);
	Result.DefaultButton.Pressed = model::FlatGradient(Palette.ButtonPressed);
	Result.DefaultButton.Disabled = model::FlatGradient(Palette.ButtonDisabled);
	Result.DefaultButton.Border = Palette.ButtonBorder;
	Result.DefaultButton.Text = Palette.TextNormal;
	if ( const model::IConfigNode* const Styles = InNode ? InNode->FindChild("button_styles") : nullptr )
	{
		for ( const std::string& Id : Styles->GetKeys() )
		{
			const ButtonStyle Style = ReadButtonStyle(*Styles->FindChild(Id), Palette, Result.DefaultButton);
			if ( Id == "default" )
			{
				Result.DefaultButton = Style;
				continue;
			}
			Result.ButtonStyles.EmplaceBack(Id, Style);
		}
	}

	const model::ConfigFields Button = Root.Child("button");
	Button.Read("height", Result.ButtonHeight);
	Button.Read("border", Result.BorderWidth);


	const model::ConfigFields Toast = Root.Child("toast");
	Toast.Read("font_size", Result.ToastFontSize);
	Toast.Read("show_time", Result.ToastShowTime);
	Toast.Read("fade_time", Result.ToastFadeTime);
	Root.Read("scene_fade_time", Result.SceneFadeTime);
	const model::ConfigFields Hints = Root.Child("hints");
	Hints.Read("margin", Result.HintsMargin);
	return Result;
}

} // namespace game::viewmodel

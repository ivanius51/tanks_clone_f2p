#include "viewmodel/WidgetDef.h"
#include "templates/TContainers.h"

#include <cmath>

namespace game::viewmodel
{

bool EnumFromName(const std::string_view InName, WidgetPart& OutPart)
{
	if ( InName == "widget" ) { OutPart = WidgetPart::Widget; return true; }
	if ( InName == "label" ) { OutPart = WidgetPart::Label; return true; }
	if ( InName == "icon" ) { OutPart = WidgetPart::Icon; return true; }
	if ( InName == "rect" ) { OutPart = WidgetPart::Rect; return true; }
	if ( InName == "ring" ) { OutPart = WidgetPart::Ring; return true; }
	if ( InName == "bar" ) { OutPart = WidgetPart::Bar; return true; }
	if ( InName == "spacer" ) { OutPart = WidgetPart::Spacer; return true; }
	return false;
}

bool ColorRef::FromConfig(const model::IConfigNode& InNode)
{
	if ( InNode.GetItemCount() > 0 )
	{
		return bLiteral = InNode.ReadSelf(Literal);
	}
	return InNode.ReadSelf(Name);
}

namespace
{
	model::JsonValue Num(const float InValue)
	{
		const double Rounded = std::round(static_cast<double>(InValue) * 10000.0) / 10000.0;
		return Rounded == std::floor(Rounded) && std::abs(Rounded) < 1.0e9 ? model::JsonValue(static_cast<int>(Rounded)) : model::JsonValue(Rounded);
	}

	model::JsonValue Pair(const model::Vec2f& InValue)
	{
		return model::JsonValue(model::JsonValue::Array{ Num(InValue.X), Num(InValue.Y) });
	}

	std::string_view NameOf(const model::LayoutKind InKind)
	{
		switch ( InKind )
		{
			case model::LayoutKind::Row: return "row";
			case model::LayoutKind::Column: return "column";
			case model::LayoutKind::Anchor: return "anchor";
			case model::LayoutKind::Grid: return "grid";
			default: return "stack";
		}
	}

	std::string_view NameOf(const model::LayoutAlign InAlign)
	{
		switch ( InAlign )
		{
			case model::LayoutAlign::Start: return "start";
			case model::LayoutAlign::End: return "end";
			default: return "center";
		}
	}

	std::string_view NameOf(const WidgetPart InPart)
	{
		switch ( InPart )
		{
			case WidgetPart::Widget: return "widget";
			case WidgetPart::Label: return "label";
			case WidgetPart::Icon: return "icon";
			case WidgetPart::Rect: return "rect";
			case WidgetPart::Ring: return "ring";
			case WidgetPart::Bar: return "bar";
			default: return "spacer";
		}
	}

	model::JsonValue CopyOf(const model::IConfigNode& InNode)
	{
		if ( InNode.GetItemCount() > 0 )
		{
			model::JsonValue::Array Items;
			for ( std::size_t Index = 0; Index < InNode.GetItemCount(); ++Index )
			{
				Items.PushBack(CopyOf(*InNode.GetItem(Index)));
			}
			return model::JsonValue(std::move(Items));
		}
		if ( const std::optional<bool> Flag = InNode.ReadBool() )
		{
			return model::JsonValue(*Flag);
		}
		if ( const std::optional<double> Number = InNode.ReadNumber() )
		{
			return *Number == std::floor(*Number) && std::abs(*Number) < 1.0e9 ? model::JsonValue(static_cast<int>(*Number)) : model::JsonValue(*Number);
		}
		if ( const std::optional<std::string> Text = InNode.ReadString() )
		{
			return model::JsonValue(*Text);
		}
		model::JsonValue::Object Members;
		for ( const std::string& Key : InNode.GetKeys() )
		{
			Members[Key] = CopyOf(*InNode.FindChild(Key));
		}
		return model::JsonValue(std::move(Members));
	}

	void ReadChildKeys(const model::IConfigNode& InNode, WidgetChildDef& Child)
	{
		InNode.TryRead("id", Child.Id);
		InNode.TryRead("widget", Child.Widget);
		InNode.TryRead("text", Child.Text);
		InNode.TryRead("text_key", Child.TextKey);
		InNode.TryRead("frame", Child.Frame);
		InNode.TryRead("font_size", Child.FontSize);
		InNode.TryRead("decorative", Child.bDecorative);
		InNode.TryRead("text_color", Child.TextColor);
		InNode.TryRead("fill", Child.FillColor);
		InNode.TryRead("border", Child.BorderColor);
		InNode.TryRead("back", Child.BackColor);
		InNode.TryRead("anim_time", Child.AnimTime);
		InNode.TryRead("trail", Child.TrailColor);
		InNode.TryRead("trail_delay", Child.TrailDelay);
		InNode.TryRead("anim_easing", Child.AnimEase);
		InNode.TryRead("border_width", Child.BorderWidth);
		InNode.TryRead("shrink", Child.bShrinkToFit);
		InNode.TryRead("max_width", Child.MaxWidth);
		InNode.TryRead("min_scale", Child.MinScale);
		InNode.TryRead("max_chars", Child.MaxChars);
		InNode.TryRead("wrap", Child.bWrap);
		InNode.TryRead("size", Child.Size);
		InNode.TryRead("anchor", Child.Anchor);
		InNode.TryRead("offset", Child.Offset);
		if ( InNode.TryRead("pivot", Child.Pivot) )
		{
			Child.bHasPivot = true;
		}
		InNode.TryRead("radius", Child.Radius);
		InNode.TryRead("fill_parent", Child.bFill);
		InNode.TryRead("grow", Child.Grow);
		InNode.TryRead("stretch", Child.bStretch);
		InNode.TryRead("opacity", Child.Opacity);
		InNode.TryRead("visible", Child.bVisible);
		InNode.TryRead("rotation", Child.Rotation);
		InNode.TryRead("scale", Child.Scale);
		InNode.TryRead("z", Child.ZOrder);
		InNode.TryRead("animation", Child.Animation);
		InNode.TryRead("hover_fill", Child.HoverFill);
		InNode.TryRead("hover_border", Child.HoverBorder);
		InNode.TryRead("hover_opacity", Child.HoverOpacity);
	}

	WidgetChildDef ReadChild(const model::IConfigNode& InNode, const std::string& InPath, TArray<std::string>& OutMissing)
	{
		WidgetChildDef Child;
		if ( !InNode.TryRead("type", Child.Part) )
		{
			OutMissing.PushBack(InPath + "type");
		}
		ReadChildKeys(InNode, Child);
		if ( Child.Part == WidgetPart::Widget && Child.Widget.empty() )
		{
			OutMissing.PushBack(InPath + "widget");
		}
		return Child;
	}

	void ReadRules(const model::IConfigNode& InNode, WidgetDef& OutDef)
	{
		InNode.TryRead("layout", OutDef.Rules.Kind);
		InNode.TryRead("gap", OutDef.Rules.Gap);
		InNode.TryRead("padding", OutDef.Rules.Padding);
		InNode.TryRead("size", OutDef.Rules.FixedSize);
		InNode.TryRead("min_size", OutDef.Rules.MinSize);
		InNode.TryRead("main_align", OutDef.Rules.MainAlign);
		InNode.TryRead("cross_align", OutDef.Rules.CrossAlign);
		InNode.TryRead("columns", OutDef.Rules.Columns);
		InNode.TryRead("on_click", OutDef.OnClick);
		InNode.TryRead("scale_with_text", OutDef.bScaleWithText);
		InNode.TryRead("layer", OutDef.Layer);
	}

	void Put(model::JsonValue::Object& OutObject, const char* const InKey, model::JsonValue InValue)
	{
		OutObject[InKey] = std::move(InValue);
	}

	void PutColor(model::JsonValue::Object& OutObject, const char* const InKey, const ColorRef& InColor, const bool bInSkipDefaultText = false)
	{
		if ( !InColor.IsSet() || (bInSkipDefaultText && !InColor.bLiteral && InColor.Name == "text") )
		{
			return;
		}
		OutObject[InKey] = InColor.ToConfig();
	}
}

model::JsonValue ColorRef::ToConfig() const
{
	if ( bLiteral )
	{
		return model::JsonValue(model::JsonValue::Array{ model::JsonValue(static_cast<int>(Literal.R)), model::JsonValue(static_cast<int>(Literal.G)),
			model::JsonValue(static_cast<int>(Literal.B)), model::JsonValue(static_cast<int>(Literal.A)) });
	}
	return model::JsonValue(Name);
}

model::JsonValue WidgetChildDef::ToConfig() const
{
	const WidgetChildDef Default;
	model::JsonValue::Object Out;
	Put(Out, "type", model::JsonValue(std::string(NameOf(Part))));
	const auto PutText = [&Out](const char* const InKey, const std::string& InValue)
	{
		if ( !InValue.empty() )
		{
			Out[InKey] = model::JsonValue(InValue);
		}
	};
	const auto PutNumber = [&Out](const char* const InKey, const float InValue, const float InDefault)
	{
		if ( InValue != InDefault )
		{
			Out[InKey] = Num(InValue);
		}
	};
	const auto PutFlag = [&Out](const char* const InKey, const bool bInValue, const bool bInDefault)
	{
		if ( bInValue != bInDefault )
		{
			Out[InKey] = model::JsonValue(bInValue);
		}
	};
	PutText("id", Id);
	PutText("widget", Widget);
	PutText("text", Text);
	PutText("text_key", TextKey);
	PutText("frame", Frame);
	PutNumber("font_size", FontSize, Default.FontSize);
	PutFlag("decorative", bDecorative, Default.bDecorative);
	PutColor(Out, "text_color", TextColor, true);
	PutColor(Out, "fill", FillColor);
	PutColor(Out, "border", BorderColor);
	PutColor(Out, "back", BackColor);
	PutNumber("anim_time", AnimTime, Default.AnimTime);
	PutColor(Out, "trail", TrailColor);
	PutNumber("trail_delay", TrailDelay, Default.TrailDelay);
	if ( AnimEase != Default.AnimEase )
	{
		Out["anim_easing"] = model::JsonValue(std::string(NameOf(AnimEase)));
	}
	PutNumber("border_width", BorderWidth, Default.BorderWidth);
	PutFlag("shrink", bShrinkToFit, Default.bShrinkToFit);
	PutNumber("max_width", MaxWidth, Default.MaxWidth);
	PutNumber("min_scale", MinScale, Default.MinScale);
	PutNumber("max_chars", static_cast<float>(MaxChars), 0.0f);
	PutFlag("wrap", bWrap, Default.bWrap);
	if ( Size.X != 0.0f || Size.Y != 0.0f )
	{
		Out["size"] = Pair(Size);
	}
	if ( Anchor.X != Default.Anchor.X || Anchor.Y != Default.Anchor.Y )
	{
		Out["anchor"] = Pair(Anchor);
	}
	if ( Offset.X != 0.0f || Offset.Y != 0.0f )
	{
		Out["offset"] = Pair(Offset);
	}
	if ( bHasPivot )
	{
		Out["pivot"] = Pair(Pivot);
	}
	PutNumber("radius", Radius, Default.Radius);
	PutFlag("fill_parent", bFill, Default.bFill);
	PutNumber("grow", Grow, Default.Grow);
	PutFlag("stretch", bStretch, Default.bStretch);
	PutNumber("opacity", Opacity, Default.Opacity);
	PutFlag("visible", bVisible, Default.bVisible);
	PutNumber("rotation", Rotation, Default.Rotation);
	PutNumber("scale", Scale, Default.Scale);
	PutNumber("z", static_cast<float>(ZOrder), 0.0f);
	PutText("animation", Animation);
	PutColor(Out, "hover_fill", HoverFill);
	PutColor(Out, "hover_border", HoverBorder);
	PutNumber("hover_opacity", HoverOpacity, Default.HoverOpacity);
	return model::JsonValue(std::move(Out));
}

WidgetDef WidgetDef::FromConfig(const model::IConfigNode& InNode, const std::string& InId, TArray<std::string>& OutMissing, const std::string_view InVariant)
{
	WidgetDef Result;
	Result.Id = InId;
	ReadRules(InNode, Result);

	const model::IConfigNode* const Children = InNode.FindChild("children");
	for ( std::size_t Index = 0; Children && Index < Children->GetItemCount(); ++Index )
	{
		Result.Children.PushBack(ReadChild(*Children->GetItem(Index), InId + ".children[" + std::to_string(Index) + "].", OutMissing));
	}

	const model::IConfigNode* const Variants = InNode.FindChild("variants");
	if ( Variants )
	{
		Result.Variants = CopyOf(*Variants);
	}
	const model::IConfigNode* const Chosen = Variants && !InVariant.empty() ? Variants->FindChild(InVariant) : nullptr;
	if ( Chosen )
	{
		ReadRules(*Chosen, Result);
		const model::IConfigNode* const Overrides = Chosen->FindChild("children");
		for ( const std::string& ChildId : Overrides ? Overrides->GetKeys() : TArray<std::string>() )
		{
			for ( WidgetChildDef& Current : Result.Children )
			{
				if ( Current.Id == ChildId )
				{
					ReadChildKeys(*Overrides->FindChild(ChildId), Current);
				}
			}
		}
	}
	return Result;
}

model::JsonValue WidgetDef::ToConfig() const
{
	const model::LayoutRules Default;
	model::JsonValue::Object Out;
	if ( Rules.Kind != Default.Kind )
	{
		Out["layout"] = model::JsonValue(std::string(NameOf(Rules.Kind)));
	}
	if ( Rules.Gap != 0.0f )
	{
		Out["gap"] = Num(Rules.Gap);
	}
	if ( Rules.Padding.X != 0.0f || Rules.Padding.Y != 0.0f )
	{
		Out["padding"] = Pair(Rules.Padding);
	}
	if ( Rules.FixedSize.X != 0.0f || Rules.FixedSize.Y != 0.0f )
	{
		Out["size"] = Pair(Rules.FixedSize);
	}
	if ( Rules.MinSize.X != 0.0f || Rules.MinSize.Y != 0.0f )
	{
		Out["min_size"] = Pair(Rules.MinSize);
	}
	if ( Rules.MainAlign != Default.MainAlign )
	{
		Out["main_align"] = model::JsonValue(std::string(NameOf(Rules.MainAlign)));
	}
	if ( Rules.CrossAlign != Default.CrossAlign )
	{
		Out["cross_align"] = model::JsonValue(std::string(NameOf(Rules.CrossAlign)));
	}
	if ( Rules.Columns != Default.Columns )
	{
		Out["columns"] = model::JsonValue(Rules.Columns);
	}
	if ( !OnClick.empty() )
	{
		Out["on_click"] = model::JsonValue(OnClick);
	}
	if ( bScaleWithText )
	{
		Out["scale_with_text"] = model::JsonValue(true);
	}
	if ( Layer != 0 )
	{
		Out["layer"] = model::JsonValue(Layer);
	}
	model::JsonValue::Array Items;
	for ( const WidgetChildDef& Current : Children )
	{
		Items.PushBack(Current.ToConfig());
	}
	Out["children"] = model::JsonValue(std::move(Items));
	if ( Variants.IsObject() )
	{
		Out["variants"] = Variants;
	}
	return model::JsonValue(std::move(Out));
}

} // namespace game::viewmodel

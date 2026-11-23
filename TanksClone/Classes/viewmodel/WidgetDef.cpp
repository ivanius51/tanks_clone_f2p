#include "viewmodel/WidgetDef.h"

namespace tanks::viewmodel
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
	WidgetChildDef ReadChild(const model::IConfigNode& InNode, const std::string& InPath, std::vector<std::string>& OutMissing)
	{
		WidgetChildDef Child;
		if ( !InNode.TryRead("type", Child.Part) )
		{
			OutMissing.push_back(InPath + "type");
		}
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
		InNode.TryRead("border_width", Child.BorderWidth);
		InNode.TryRead("shrink", Child.bShrinkToFit);
		InNode.TryRead("wrap", Child.bWrap);
		InNode.TryRead("size", Child.Size);
		InNode.TryRead("anchor", Child.Anchor);
		InNode.TryRead("offset", Child.Offset);
	Child.bHasPivot = InNode.TryRead("pivot", Child.Pivot);
	InNode.TryRead("radius", Child.Radius);
		InNode.TryRead("fill_parent", Child.bFill);
	InNode.TryRead("grow", Child.Grow);
	InNode.TryRead("stretch", Child.bStretch);
	InNode.TryRead("opacity", Child.Opacity);
		if ( Child.Part == WidgetPart::Widget && Child.Widget.empty() )
		{
			OutMissing.push_back(InPath + "widget");
		}
		return Child;
	}
}

WidgetDef WidgetDef::FromConfig(const model::IConfigNode& InNode, const std::string& InId, std::vector<std::string>& OutMissing)
{
	WidgetDef Result;
	Result.Id = InId;
	InNode.TryRead("layout", Result.Rules.Kind);
	InNode.TryRead("gap", Result.Rules.Gap);
	InNode.TryRead("padding", Result.Rules.Padding);
	InNode.TryRead("size", Result.Rules.FixedSize);
	InNode.TryRead("min_size", Result.Rules.MinSize);
	InNode.TryRead("main_align", Result.Rules.MainAlign);
	InNode.TryRead("cross_align", Result.Rules.CrossAlign);
	InNode.TryRead("columns", Result.Rules.Columns);
	InNode.TryRead("on_click", Result.OnClick);
	InNode.TryRead("scale_with_text", Result.bScaleWithText);

	const model::IConfigNode* const Children = InNode.FindChild("children");
	for ( std::size_t Index = 0; Children && Index < Children->GetItemCount(); ++Index )
	{
		Result.Children.push_back(ReadChild(*Children->GetItem(Index), InId + ".children[" + std::to_string(Index) + "].", OutMissing));
	}
	return Result;
}

} // namespace tanks::viewmodel

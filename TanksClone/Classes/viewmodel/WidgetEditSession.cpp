#include "viewmodel/WidgetEditSession.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace tanks::viewmodel
{

namespace
{
	struct PropertyInfo
	{
		EditProperty Property;
		const char* Name;
		bool bOfChild;
		float Step;
		float Min;
		float Max;
	};

	constexpr float NoLimit = 100000.0f;

	constexpr std::array<PropertyInfo, 20> Infos{ {
		{ EditProperty::Gap, "gap", false, 1.0f, 0.0f, NoLimit },
		{ EditProperty::PaddingX, "padding x", false, 1.0f, 0.0f, NoLimit },
		{ EditProperty::PaddingY, "padding y", false, 1.0f, 0.0f, NoLimit },
		{ EditProperty::FixedWidth, "width", false, 4.0f, 0.0f, NoLimit },
		{ EditProperty::FixedHeight, "height", false, 4.0f, 0.0f, NoLimit },
		{ EditProperty::SizeX, "size x", true, 2.0f, 0.0f, NoLimit },
		{ EditProperty::SizeY, "size y", true, 2.0f, 0.0f, NoLimit },
		{ EditProperty::OffsetX, "offset x", true, 2.0f, -NoLimit, NoLimit },
		{ EditProperty::OffsetY, "offset y", true, 2.0f, -NoLimit, NoLimit },
		{ EditProperty::FontSize, "font size", true, 1.0f, 4.0f, 200.0f },
		{ EditProperty::Opacity, "opacity", true, 0.1f, 0.0f, 1.0f },
		{ EditProperty::Rotation, "rotation", true, 5.0f, -360.0f, 360.0f },
		{ EditProperty::Scale, "scale", true, 0.1f, 0.1f, 10.0f },
		{ EditProperty::Grow, "grow", true, 0.5f, 0.0f, 10.0f },
		{ EditProperty::Radius, "radius", true, 2.0f, 0.0f, NoLimit },
		{ EditProperty::BorderWidth, "border width", true, 1.0f, 0.0f, 100.0f },
		{ EditProperty::MinWidth, "min width", false, 4.0f, 0.0f, NoLimit },
		{ EditProperty::MinHeight, "min height", false, 4.0f, 0.0f, NoLimit },
		{ EditProperty::AnchorX, "anchor x", true, 0.1f, 0.0f, 1.0f },
		{ EditProperty::AnchorY, "anchor y", true, 0.1f, 0.0f, 1.0f } } };

	const PropertyInfo& InfoOf(const EditProperty InProperty)
	{
		return Infos[static_cast<size_t>(InProperty)];
	}

	bool IsInOrder()
	{
		for ( size_t Index = 0; Index < Infos.size(); ++Index )
		{
			if ( static_cast<size_t>(Infos[Index].Property) != Index )
			{
				return false;
			}
		}
		return true;
	}
	static_assert(static_cast<size_t>(EditProperty::AnchorY) + 1 == Infos.size(), "one entry per property");
}

WidgetEditSession::WidgetEditSession(WidgetDef InDef)
	: Def(std::move(InDef))
{
	SavedText = ToText();
}

const std::vector<EditProperty>& WidgetEditSession::GetProperties()
{
	static const std::vector<EditProperty> All = []()
	{
		std::vector<EditProperty> Result;
		for ( const PropertyInfo& Current : Infos )
		{
			Result.push_back(Current.Property);
		}
		return Result;
	}();
	return All;
}

std::string WidgetEditSession::GetPropertyName(const EditProperty InProperty)
{
	return IsInOrder() ? InfoOf(InProperty).Name : std::string();
}

const WidgetDef& WidgetEditSession::GetDef() const
{
	return Def;
}

int WidgetEditSession::GetSelected() const
{
	return Selected;
}

void WidgetEditSession::Select(const int InIndex)
{
	Selected = InIndex >= 0 && InIndex < static_cast<int>(Def.Children.size()) ? InIndex : WidgetSelected;
}

void WidgetEditSession::SelectNext(const int InDirection)
{
	const int Count = static_cast<int>(Def.Children.size()) + 1;
	const int Position = Selected + 1;
	Select((Position + (InDirection < 0 ? Count - 1 : 1)) % Count - 1);
}

std::string WidgetEditSession::GetSelectedName() const
{
	if ( Selected == WidgetSelected )
	{
		return Def.Id.empty() ? "widget" : Def.Id;
	}
	const WidgetChildDef& Child = Def.Children[static_cast<size_t>(Selected)];
	return Child.Id.empty() ? "#" + std::to_string(Selected) : Child.Id;
}

bool WidgetEditSession::IsApplicable(const EditProperty InProperty) const
{
	if ( InfoOf(InProperty).bOfChild != (Selected != WidgetSelected) )
	{
		return false;
	}
	if ( Selected == WidgetSelected )
	{
		return true;
	}
	const WidgetPart Part = Def.Children[static_cast<size_t>(Selected)].Part;
	switch ( InProperty )
	{
		case EditProperty::FontSize: return Part == WidgetPart::Label;
		case EditProperty::Radius: return Part == WidgetPart::Rect;
		case EditProperty::BorderWidth: return Part == WidgetPart::Rect || Part == WidgetPart::Ring;
		default: return true;
	}
}

float* WidgetEditSession::FindNumber(const EditProperty InProperty)
{
	WidgetChildDef* const Child = Selected == WidgetSelected ? nullptr : &Def.Children[static_cast<size_t>(Selected)];
	switch ( InProperty )
	{
		case EditProperty::Gap: return &Def.Rules.Gap;
		case EditProperty::PaddingX: return &Def.Rules.Padding.X;
		case EditProperty::PaddingY: return &Def.Rules.Padding.Y;
		case EditProperty::FixedWidth: return &Def.Rules.FixedSize.X;
		case EditProperty::FixedHeight: return &Def.Rules.FixedSize.Y;
		case EditProperty::MinWidth: return &Def.Rules.MinSize.X;
		case EditProperty::MinHeight: return &Def.Rules.MinSize.Y;
		default: break;
	}
	if ( !Child )
	{
		return nullptr;
	}
	switch ( InProperty )
	{
		case EditProperty::SizeX: return &Child->Size.X;
		case EditProperty::SizeY: return &Child->Size.Y;
		case EditProperty::OffsetX: return &Child->Offset.X;
		case EditProperty::OffsetY: return &Child->Offset.Y;
		case EditProperty::FontSize: return &Child->FontSize;
		case EditProperty::Opacity: return &Child->Opacity;
		case EditProperty::Rotation: return &Child->Rotation;
		case EditProperty::Scale: return &Child->Scale;
		case EditProperty::Grow: return &Child->Grow;
		case EditProperty::Radius: return &Child->Radius;
		case EditProperty::BorderWidth: return &Child->BorderWidth;
		case EditProperty::AnchorX: return &Child->Anchor.X;
		case EditProperty::AnchorY: return &Child->Anchor.Y;
		default: return nullptr;
	}
}

const float* WidgetEditSession::FindNumber(const EditProperty InProperty) const
{
	return const_cast<WidgetEditSession*>(this)->FindNumber(InProperty);
}

float WidgetEditSession::GetValue(const EditProperty InProperty) const
{
	const float* const Value = IsApplicable(InProperty) ? FindNumber(InProperty) : nullptr;
	return Value ? *Value : 0.0f;
}

void WidgetEditSession::Nudge(const EditProperty InProperty, const int InDirection)
{
	float* const Value = IsApplicable(InProperty) ? FindNumber(InProperty) : nullptr;
	if ( !Value || InDirection == 0 )
	{
		return;
	}
	const PropertyInfo& Info = InfoOf(InProperty);
	const float Next = std::clamp(*Value + (InDirection > 0 ? Info.Step : -Info.Step), Info.Min, Info.Max);
	if ( Next == *Value )
	{
		return;
	}
	PushUndo();
	*Value = std::round(Next * 100.0f) / 100.0f;
}

void WidgetEditSession::PushUndo()
{
	UndoStack.push_back({ Def, Selected });
	RedoStack.clear();
}

namespace
{
	constexpr std::array<const char*, 5> LayoutNames = { "stack", "row", "column", "anchor", "grid" };
	constexpr std::array<const char*, 3> AlignNames = { "start", "center", "end" };
	constexpr std::array<std::pair<EditChoice, const char*>, 8> ChoiceNames = { {
		{ EditChoice::Layout, "layout" },
		{ EditChoice::MainAlign, "main align" },
		{ EditChoice::CrossAlign, "cross align" },
		{ EditChoice::ScaleWithText, "scale with text" },
		{ EditChoice::Fill, "fill parent" },
		{ EditChoice::Stretch, "stretch" },
		{ EditChoice::Shrink, "shrink to fit" },
		{ EditChoice::Wrap, "wrap" }
	} };
	static_assert(static_cast<size_t>(model::LayoutKind::Grid) + 1 == LayoutNames.size(), "one name per layout kind");
	static_assert(static_cast<size_t>(model::LayoutAlign::End) + 1 == AlignNames.size(), "one name per alignment");

	bool IsWidgetChoice(const EditChoice InChoice)
	{
		return InChoice <= EditChoice::ScaleWithText;
	}
}

const std::vector<EditChoice>& WidgetEditSession::GetChoices()
{
	static const std::vector<EditChoice> All = []()
	{
		std::vector<EditChoice> Result;
		for ( const auto& Current : ChoiceNames )
		{
			Result.push_back(Current.first);
		}
		return Result;
	}();
	return All;
}

std::string WidgetEditSession::GetChoiceName(const EditChoice InChoice)
{
	return ChoiceNames[static_cast<size_t>(InChoice)].second;
}

bool WidgetEditSession::IsChoiceApplicable(const EditChoice InChoice) const
{
	if ( IsWidgetChoice(InChoice) )
	{
		return Selected == WidgetSelected;
	}
	if ( Selected == WidgetSelected )
	{
		return false;
	}
	const bool bLabel = Def.Children[static_cast<size_t>(Selected)].Part == WidgetPart::Label;
	return (InChoice != EditChoice::Shrink && InChoice != EditChoice::Wrap) || bLabel;
}

std::string WidgetEditSession::GetChoiceValue(const EditChoice InChoice) const
{
	if ( !IsChoiceApplicable(InChoice) )
	{
		return std::string();
	}
	const auto OnOff = [](const bool bInValue) { return std::string(bInValue ? "on" : "off"); };
	const WidgetChildDef* const Child = Selected == WidgetSelected ? nullptr : &Def.Children[static_cast<size_t>(Selected)];
	switch ( InChoice )
	{
		case EditChoice::Layout: return LayoutNames[static_cast<size_t>(Def.Rules.Kind)];
		case EditChoice::MainAlign: return AlignNames[static_cast<size_t>(Def.Rules.MainAlign)];
		case EditChoice::CrossAlign: return AlignNames[static_cast<size_t>(Def.Rules.CrossAlign)];
		case EditChoice::ScaleWithText: return OnOff(Def.bScaleWithText);
		case EditChoice::Fill: return OnOff(Child->bFill);
		case EditChoice::Stretch: return OnOff(Child->bStretch);
		case EditChoice::Shrink: return OnOff(Child->bShrinkToFit);
		case EditChoice::Wrap: return OnOff(Child->bWrap);
	}
	return std::string();
}

void WidgetEditSession::CycleChoice(const EditChoice InChoice, const int InDirection)
{
	if ( !IsChoiceApplicable(InChoice) || InDirection == 0 )
	{
		return;
	}
	PushUndo();
	const int Step = InDirection > 0 ? 1 : -1;
	const auto Next = [Step](const int InCurrent, const int InCount) { return (InCurrent + Step + InCount) % InCount; };
	WidgetChildDef* const Child = Selected == WidgetSelected ? nullptr : &Def.Children[static_cast<size_t>(Selected)];
	switch ( InChoice )
	{
		case EditChoice::Layout: Def.Rules.Kind = static_cast<model::LayoutKind>(Next(static_cast<int>(Def.Rules.Kind), static_cast<int>(LayoutNames.size()))); break;
		case EditChoice::MainAlign: Def.Rules.MainAlign = static_cast<model::LayoutAlign>(Next(static_cast<int>(Def.Rules.MainAlign), static_cast<int>(AlignNames.size()))); break;
		case EditChoice::CrossAlign: Def.Rules.CrossAlign = static_cast<model::LayoutAlign>(Next(static_cast<int>(Def.Rules.CrossAlign), static_cast<int>(AlignNames.size()))); break;
		case EditChoice::ScaleWithText: Def.bScaleWithText = !Def.bScaleWithText; break;
		case EditChoice::Fill: Child->bFill = !Child->bFill; break;
		case EditChoice::Stretch: Child->bStretch = !Child->bStretch; break;
		case EditChoice::Shrink: Child->bShrinkToFit = !Child->bShrinkToFit; break;
		case EditChoice::Wrap: Child->bWrap = !Child->bWrap; break;
	}
}

bool WidgetEditSession::CanUndo() const
{
	return !UndoStack.empty();
}

bool WidgetEditSession::CanRedo() const
{
	return !RedoStack.empty();
}

void WidgetEditSession::Undo()
{
	if ( UndoStack.empty() )
	{
		return;
	}
	RedoStack.push_back({ std::move(Def), Selected });
	Def = std::move(UndoStack.back().Def);
	Selected = UndoStack.back().Selected;
	UndoStack.pop_back();
}

void WidgetEditSession::Redo()
{
	if ( RedoStack.empty() )
	{
		return;
	}
	UndoStack.push_back({ std::move(Def), Selected });
	Def = std::move(RedoStack.back().Def);
	Selected = RedoStack.back().Selected;
	RedoStack.pop_back();
}

bool WidgetEditSession::HasTextKey() const
{
	return Selected != WidgetSelected && Def.Children[static_cast<size_t>(Selected)].Part == WidgetPart::Label;
}

const std::string& WidgetEditSession::GetTextKey() const
{
	static const std::string None;
	return HasTextKey() ? Def.Children[static_cast<size_t>(Selected)].TextKey : None;
}

void WidgetEditSession::SetTextKey(const std::string& InKey)
{
	if ( !HasTextKey() || Def.Children[static_cast<size_t>(Selected)].TextKey == InKey )
	{
		return;
	}
	PushUndo();
	Def.Children[static_cast<size_t>(Selected)].TextKey = InKey;
}

bool WidgetEditSession::IsDirty() const
{
	return ToText() != SavedText;
}

void WidgetEditSession::MarkSaved()
{
	SavedText = ToText();
}

std::string WidgetEditSession::ToText() const
{
	return Def.ToConfig().ToString();
}

} // namespace tanks::viewmodel

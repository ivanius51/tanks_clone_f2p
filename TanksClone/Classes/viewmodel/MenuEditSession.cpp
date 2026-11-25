#include "viewmodel/MenuEditSession.h"

#include <algorithm>
#include <array>
#include <utility>

namespace game::viewmodel
{

namespace
{
	constexpr std::array<std::pair<MenuField, const char*>, 6> FieldNames = { {
		{ MenuField::Kind, "kind" },
		{ MenuField::TextKey, "text" },
		{ MenuField::Value, "value" },
		{ MenuField::Action, "action" },
		{ MenuField::When, "when" },
		{ MenuField::EnabledWhen, "enabled when" }
	} };
}

MenuEditSession::MenuEditSession(MenuDef InDef)
	: Def(std::move(InDef))
{
	SavedText = ToText();
}

const std::vector<MenuField>& MenuEditSession::GetFields()
{
	static const std::vector<MenuField> All = []()
	{
		std::vector<MenuField> Result;
		for ( const auto& Current : FieldNames )
		{
			Result.push_back(Current.first);
		}
		return Result;
	}();
	return All;
}

std::string MenuEditSession::GetFieldName(const MenuField InField)
{
	for ( const auto& Current : FieldNames )
	{
		if ( Current.first == InField )
		{
			return Current.second;
		}
	}
	return std::string();
}

const MenuDef& MenuEditSession::GetDef() const
{
	return Def;
}

int MenuEditSession::GetSelected() const
{
	return Selected;
}

void MenuEditSession::Select(const int InIndex)
{
	bStyleSelected = false;
	Selected = InIndex >= 0 && InIndex < static_cast<int>(Def.Items.size()) ? InIndex : NoneSelected;
}

bool MenuEditSession::HasSelection() const
{
	return Selected != NoneSelected;
}

void MenuEditSession::SelectStyle()
{
	Selected = NoneSelected;
	bStyleSelected = true;
}

bool MenuEditSession::IsStyleSelected() const
{
	return bStyleSelected;
}

float MenuEditSession::GetMetric(const MenuMetric InMetric) const
{
	const MenuButtonMetrics& Button = Def.Button;
	return InMetric == MenuMetric::ButtonWidth ? Button.Width : InMetric == MenuMetric::ButtonHeight ? Button.Height : Button.Spacing;
}

void MenuEditSession::NudgeMetric(const MenuMetric InMetric, const int InDirection)
{
	if ( InDirection == 0 )
	{
		return;
	}
	PushUndo();
	MenuButtonMetrics& Button = Def.Button;
	float& Value = InMetric == MenuMetric::ButtonWidth ? Button.Width : InMetric == MenuMetric::ButtonHeight ? Button.Height : Button.Spacing;
	Value = std::max(0.0f, Value + (InMetric == MenuMetric::ButtonSpacing ? 1.0f : 4.0f) * static_cast<float>(InDirection));
}

std::string* MenuEditSession::FindField(const MenuField InField)
{
	return const_cast<std::string*>(std::as_const(*this).FindField(InField));
}

const std::string* MenuEditSession::FindField(const MenuField InField) const
{
	if ( !HasSelection() )
	{
		return nullptr;
	}
	const MenuItemDef& Item = Def.Items[static_cast<size_t>(Selected)];
	switch ( InField )
	{
	case MenuField::Kind: return &Item.Kind;
	case MenuField::TextKey: return &Item.TextKey;
	case MenuField::Value: return &Item.ValueId;
	case MenuField::Action: return &Item.Action;
	case MenuField::When: return &Item.When;
	case MenuField::EnabledWhen: return &Item.EnabledWhen;
	}
	return nullptr;
}

std::string MenuEditSession::GetValue(const MenuField InField) const
{
	const std::string* const Found = FindField(InField);
	return Found ? *Found : std::string();
}

void MenuEditSession::PushUndo()
{
	UndoStack.push_back({ Def, Selected });
	RedoStack.clear();
}

void MenuEditSession::Cycle(const MenuField InField, const int InDirection, const std::vector<std::string>& InChoices)
{
	std::string* const Field = FindField(InField);
	if ( !Field )
	{
		return;
	}
	std::vector<std::string> Options{ std::string() };
	Options.insert(Options.end(), InChoices.begin(), InChoices.end());
	const auto Current = std::find(Options.begin(), Options.end(), *Field);
	const int Count = static_cast<int>(Options.size());
	const int From = Current == Options.end() ? 0 : static_cast<int>(Current - Options.begin());
	const int To = (((From + InDirection) % Count) + Count) % Count;
	if ( Options[static_cast<size_t>(To)] == *Field )
	{
		return;
	}
	PushUndo();
	*FindField(InField) = Options[static_cast<size_t>(To)];
}

void MenuEditSession::SetValue(const MenuField InField, const std::string& InValue)
{
	std::string* const Field = FindField(InField);
	if ( !Field || *Field == InValue )
	{
		return;
	}
	PushUndo();
	*FindField(InField) = InValue;
}

float MenuEditSession::GetNumber(const MenuNumber InNumber) const
{
	if ( !HasSelection() )
	{
		return 0.0f;
	}
	const MenuItemDef& Item = Def.Items[static_cast<size_t>(Selected)];
	return InNumber == MenuNumber::Min ? Item.Min : InNumber == MenuNumber::Max ? Item.Max : Item.Step;
}

void MenuEditSession::NudgeNumber(const MenuNumber InNumber, const int InDirection)
{
	if ( !HasSelection() )
	{
		return;
	}
	PushUndo();
	MenuItemDef& Item = Def.Items[static_cast<size_t>(Selected)];
	const float Unit = InNumber == MenuNumber::Step ? 1.0f : std::max(1.0f, Item.Step);
	float& Value = InNumber == MenuNumber::Min ? Item.Min : InNumber == MenuNumber::Max ? Item.Max : Item.Step;
	Value = std::max(0.0f, Value + InDirection * Unit);
}

void MenuEditSession::AddOption(const std::string& InOption)
{
	if ( !HasSelection() || InOption.empty() )
	{
		return;
	}
	PushUndo();
	Def.Items[static_cast<size_t>(Selected)].Options.push_back(InOption);
}

void MenuEditSession::RemoveLastOption()
{
	if ( !HasSelection() || Def.Items[static_cast<size_t>(Selected)].Options.empty() )
	{
		return;
	}
	PushUndo();
	Def.Items[static_cast<size_t>(Selected)].Options.pop_back();
}

bool MenuEditSession::IsDebug() const
{
	return HasSelection() && Def.Items[static_cast<size_t>(Selected)].bDebug;
}

void MenuEditSession::ToggleDebug()
{
	if ( !HasSelection() )
	{
		return;
	}
	PushUndo();
	MenuItemDef& Item = Def.Items[static_cast<size_t>(Selected)];
	Item.bDebug = !Item.bDebug;
}

bool MenuEditSession::CanMove(const int InDirection) const
{
	const int Target = Selected + InDirection;
	return HasSelection() && Target >= 0 && Target < static_cast<int>(Def.Items.size());
}

void MenuEditSession::MoveSelected(const int InDirection)
{
	if ( !CanMove(InDirection) )
	{
		return;
	}
	PushUndo();
	std::swap(Def.Items[static_cast<size_t>(Selected)], Def.Items[static_cast<size_t>(Selected + InDirection)]);
	Selected += InDirection;
}

std::string MenuEditSession::MakeUniqueId(const std::string& InBase) const
{
	for ( int Number = 2;; ++Number )
	{
		const std::string Candidate = InBase + "_" + std::to_string(Number);
		const bool bTaken = std::any_of(Def.Items.begin(), Def.Items.end(), [&Candidate](const MenuItemDef& InItem) { return InItem.Id == Candidate; });
		if ( !bTaken )
		{
			return Candidate;
		}
	}
}

void MenuEditSession::DuplicateSelected()
{
	if ( !HasSelection() )
	{
		return;
	}
	PushUndo();
	MenuItemDef Copy = Def.Items[static_cast<size_t>(Selected)];
	Copy.Id = MakeUniqueId(Copy.Id);
	Def.Items.insert(Def.Items.begin() + Selected + 1, Copy);
	++Selected;
}

void MenuEditSession::RemoveSelected()
{
	if ( !HasSelection() )
	{
		return;
	}
	PushUndo();
	Def.Items.erase(Def.Items.begin() + Selected);
	Selected = Selected < static_cast<int>(Def.Items.size()) ? Selected : static_cast<int>(Def.Items.size()) - 1;
	if ( Def.Items.empty() )
	{
		Selected = NoneSelected;
	}
}

bool MenuEditSession::CanUndo() const
{
	return !UndoStack.empty();
}

bool MenuEditSession::CanRedo() const
{
	return !RedoStack.empty();
}

void MenuEditSession::Undo()
{
	if ( !CanUndo() )
	{
		return;
	}
	RedoStack.push_back({ Def, Selected });
	Def = UndoStack.back().Def;
	Selected = UndoStack.back().Selected;
	UndoStack.pop_back();
}

void MenuEditSession::Redo()
{
	if ( !CanRedo() )
	{
		return;
	}
	UndoStack.push_back({ Def, Selected });
	Def = RedoStack.back().Def;
	Selected = RedoStack.back().Selected;
	RedoStack.pop_back();
}

bool MenuEditSession::IsDirty() const
{
	return ToText() != SavedText;
}

void MenuEditSession::MarkSaved()
{
	SavedText = ToText();
}

std::string MenuEditSession::ToText() const
{
	return Def.ToConfig().ToString();
}

} // namespace game::viewmodel

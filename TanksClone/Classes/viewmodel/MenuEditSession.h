#pragma once

#include <string>
#include <vector>

#include "viewmodel/MenuDef.h"

namespace tanks::viewmodel
{

/** The text fields of a menu item the widget editor can change by stepping through a list of choices. */
enum class MenuField
{
	Kind,
	TextKey,
	Value,
	Action,
	When,
	EnabledWhen
};

/** The limits of a "stepper" item. */
enum class MenuNumber
{
	Min,
	Max,
	Step
};

/**
 * One menu file being edited: the selected button, changing its fields, debug mark and place in the list, adding a copy,
 * removing it, undo and redo, and the file text to save. Engine-free: the editor scene shows it and passes the clicks on.
 */
class MenuEditSession
{
public:
	static constexpr int NoneSelected = -1;

	explicit MenuEditSession(MenuDef InDef);

	static const std::vector<MenuField>& GetFields();
	static std::string GetFieldName(MenuField InField);

	const MenuDef& GetDef() const;
	int GetSelected() const;
	// An item index; out-of-range values select nothing.
	void Select(int InIndex);
	bool HasSelection() const;

	// The field of the selected item (empty when nothing is selected).
	std::string GetValue(MenuField InField) const;
	// Steps the field through "no value" and InChoices (wrapping around) in direction InDirection (+1 or -1); one undo step.
	void Cycle(MenuField InField, int InDirection, const std::vector<std::string>& InChoices);
	// Sets the field of the selected item to InValue (a picked string id or condition); one undo step.
	void SetValue(MenuField InField, const std::string& InValue);
	float GetNumber(MenuNumber InNumber) const;
	// One step up or down (Min and Max move by the item's step, at least 1); one undo step.
	void NudgeNumber(MenuNumber InNumber, int InDirection);
	// Adds a string id to the selected item's list of options, or takes the last one off.
	void AddOption(const std::string& InOption);
	void RemoveLastOption();
	bool IsDebug() const;
	void ToggleDebug();
	bool CanMove(int InDirection) const;
	void MoveSelected(int InDirection);
	// Adds a copy of the selected item (with a new unique id) after it and selects the copy.
	void DuplicateSelected();
	void RemoveSelected();

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();
	bool IsDirty() const;
	void MarkSaved();

	// The menu file as it would be saved.
	std::string ToText() const;

private:
	struct Snapshot
	{
		MenuDef Def;
		int Selected = NoneSelected;
	};

	std::string* FindField(MenuField InField);
	const std::string* FindField(MenuField InField) const;
	std::string MakeUniqueId(const std::string& InBase) const;
	void PushUndo();

	MenuDef Def;
	int Selected = NoneSelected;
	std::vector<Snapshot> UndoStack;
	std::vector<Snapshot> RedoStack;
	std::string SavedText;
};

} // namespace tanks::viewmodel

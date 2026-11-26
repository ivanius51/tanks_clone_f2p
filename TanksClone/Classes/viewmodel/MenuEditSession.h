#pragma once

#include <string>

#include "viewmodel/MenuDef.h"
#include "templates/TContainers.h"

namespace game::viewmodel
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

/** The button size numbers of a menu the widget editor can step. */
enum class MenuMetric
{
	ButtonWidth,
	ButtonHeight,
	ButtonSpacing
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

	static const TArray<MenuField>& GetFields();
	static std::string GetFieldName(MenuField InField);

	const MenuDef& GetDef() const;
	int GetSelected() const;
	void Select(int InIndex);
	bool HasSelection() const;
	void SelectStyle();
	bool IsStyleSelected() const;
	float GetMetric(MenuMetric InMetric) const;
	void NudgeMetric(MenuMetric InMetric, int InDirection);

	std::string GetValue(MenuField InField) const;
	void Cycle(MenuField InField, int InDirection, const TArray<std::string>& InChoices);
	void SetValue(MenuField InField, const std::string& InValue);
	float GetNumber(MenuNumber InNumber) const;
	void NudgeNumber(MenuNumber InNumber, int InDirection);
	void AddOption(const std::string& InOption);
	void RemoveLastOption();
	bool IsDebug() const;
	void ToggleDebug();
	bool CanMove(int InDirection) const;
	void MoveSelected(int InDirection);
	void DuplicateSelected();
	void RemoveSelected();

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();
	bool IsDirty() const;
	void MarkSaved();

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
	bool bStyleSelected = false;
	TArray<Snapshot> UndoStack;
	TArray<Snapshot> RedoStack;
	std::string SavedText;
};
} // namespace game::viewmodel

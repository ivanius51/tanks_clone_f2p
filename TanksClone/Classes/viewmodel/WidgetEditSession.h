#pragma once

#include <string>

#include "viewmodel/WidgetDef.h"
#include "templates/TContainers.h"

namespace game::viewmodel
{
/** The numbers of a widget file the widget editor can step up and down. */
enum class EditProperty
{
	Gap,
	PaddingX,
	PaddingY,
	FixedWidth,
	FixedHeight,
	SizeX,
	SizeY,
	OffsetX,
	OffsetY,
	FontSize,
	Opacity,
	Rotation,
	Scale,
	Grow,
	Radius,
	BorderWidth,
	MinWidth,
	MinHeight,
	AnchorX,
	AnchorY
};

/** The settings of a widget file or child the widget editor steps through a short list of values (a layout kind, an alignment, a flag). */
enum class EditChoice
{
	Layout,
	MainAlign,
	CrossAlign,
	ScaleWithText,
	Fill,
	Stretch,
	Shrink,
	Wrap
};

/**
 * One widget file being edited: the selected child, stepping the numbers of the widget or the child, undo and redo,
 * and the file text to save. Engine-free: the editor scene shows it and passes the clicks on.
 */
class WidgetEditSession
{
public:
	static constexpr int WidgetSelected = -1;

	explicit WidgetEditSession(WidgetDef InDef);

	static const TArray<EditProperty>& GetProperties();
	static std::string GetPropertyName(EditProperty InProperty);

	const WidgetDef& GetDef() const;
	int GetSelected() const;
	void Select(int InIndex);
	void SelectNext(int InDirection);
	std::string GetSelectedName() const;

	bool IsApplicable(EditProperty InProperty) const;
	float GetValue(EditProperty InProperty) const;
	void Nudge(EditProperty InProperty, int InDirection);

	static const TArray<EditChoice>& GetChoices();
	static std::string GetChoiceName(EditChoice InChoice);
	bool IsChoiceApplicable(EditChoice InChoice) const;
	std::string GetChoiceValue(EditChoice InChoice) const;
	void CycleChoice(EditChoice InChoice, int InDirection);

	bool HasTextKey() const;
	const std::string& GetTextKey() const;
	void SetTextKey(const std::string& InKey);

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
		WidgetDef Def;
		int Selected = WidgetSelected;
	};

	float* FindNumber(EditProperty InProperty);
	const float* FindNumber(EditProperty InProperty) const;
	void PushUndo();

	WidgetDef Def;
	int Selected = WidgetSelected;
	TArray<Snapshot> UndoStack;
	TArray<Snapshot> RedoStack;
	std::string SavedText;
};
} // namespace game::viewmodel

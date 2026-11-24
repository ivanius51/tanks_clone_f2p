#pragma once

#include <string>
#include <vector>

#include "viewmodel/WidgetDef.h"

namespace tanks::viewmodel
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
	BorderWidth
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

	static const std::vector<EditProperty>& GetProperties();
	static std::string GetPropertyName(EditProperty InProperty);

	const WidgetDef& GetDef() const;
	int GetSelected() const;
	// A child index, or WidgetSelected for the widget's own numbers; out-of-range values select the widget.
	void Select(int InIndex);
	// Moves the selection through the widget and its children, wrapping around.
	void SelectNext(int InDirection);
	std::string GetSelectedName() const;

	// A child number needs a selected child; some only make sense for some parts (font size for a label).
	bool IsApplicable(EditProperty InProperty) const;
	float GetValue(EditProperty InProperty) const;
	// One step up (+1) or down (-1), kept in the property's range; one undo step.
	void Nudge(EditProperty InProperty, int InDirection);

	// The selected child is a label: its strings id ("text_key") can be picked.
	bool HasTextKey() const;
	const std::string& GetTextKey() const;
	void SetTextKey(const std::string& InKey);

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();
	bool IsDirty() const;
	void MarkSaved();

	// The widget file as it would be saved.
	std::string ToText() const;

private:
	struct Snapshot
	{
		WidgetDef Def;
		int Selected = WidgetSelected;
	};

	float* FindNumber(EditProperty InProperty);
	const float* FindNumber(EditProperty InProperty) const;

	WidgetDef Def;
	int Selected = WidgetSelected;
	std::vector<Snapshot> UndoStack;
	std::vector<Snapshot> RedoStack;
	std::string SavedText;
};

} // namespace tanks::viewmodel

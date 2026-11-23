#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "model/EditorCommand.h"

namespace tanks::model
{

enum class EditorTool
{
	Select,
	Place,
	Eraser,
	Move
};

/**
 * What a click on the map does for the current tool. Select picks the object under the cursor, Place puts the palette
 * item down, Eraser removes, Move picks an object up and the next click puts it down. Every change is returned as a
 * command, so the editor never edits the document directly.
 */
class EditorToolController
{
public:
	EditorTool GetTool() const { return Tool; }
	// Changing the tool drops a picked-up object.
	void SetTool(EditorTool InTool);

	// Place and Eraser repeat while the pointer is dragged over cells.
	bool RepeatsOnDrag() const { return Tool == EditorTool::Place || Tool == EditorTool::Eraser; }

	// The command for a click on InCell (nullptr when the click changes nothing). InPaletteTile is the selected palette character.
	std::unique_ptr<EditorCommand> Click(const EditorDocument& InDocument, const EditorCell& InCell, char InPaletteTile);
	// Picks up the object on InCell and switches to Move; false when the cell is empty.
	bool BeginMove(const EditorDocument& InDocument, const EditorCell& InCell);

	bool HasSelection() const { return bHasSelection; }
	const EditorCell& GetSelection() const { return Selection; }
	void ClearSelection();
	bool IsMovePending() const { return bMovePending; }

private:
	void Select(const EditorCell& InCell);

	EditorTool Tool = EditorTool::Place;
	bool bHasSelection = false;
	EditorCell Selection;
	bool bMovePending = false;
};

enum class EditorMenuAction
{
	GoToCategory,
	Remove,
	MoveObject,
	ToolSelect,
	ToolPlace,
	ToolEraser,
	ToolMove,
	Undo,
	Redo
};

/** One line of the map's right-click menu. */
struct EditorMenuItem
{
	EditorMenuAction Action = EditorMenuAction::Undo;
	bool bEnabled = true;
	bool bChecked = false;
	bool bSeparatorBefore = false;
};

struct EditorMenuContext
{
	bool bOnObject = false;
	EditorTool Tool = EditorTool::Place;
	bool bCanUndo = false;
	bool bCanRedo = false;
};

// The menu of the map: the object commands first (only on an object), then the tools, then Undo and Redo.
std::vector<EditorMenuItem> BuildEditorMenu(const EditorMenuContext& InContext);
// The string id of a menu line.
std::string_view GetEditorMenuTextKey(EditorMenuAction InAction);
// The tool a tool line of the menu selects; false for the other lines.
bool GetToolOfAction(EditorMenuAction InAction, EditorTool& OutTool);
// The string id of a history entry kind ("Add", "Remove", "Move", ...).
std::string_view GetHistoryTextKey(EditorCommandKind InKind);

} // namespace tanks::model

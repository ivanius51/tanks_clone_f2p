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
	void SetTool(EditorTool InTool);

	bool RepeatsOnDrag() const { return Tool == EditorTool::Place || Tool == EditorTool::Eraser; }

	std::unique_ptr<EditorCommand> Click(const EditorDocument& InDocument, const EditorCell& InCell, char InPaletteTile);
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

std::vector<EditorMenuItem> BuildEditorMenu(const EditorMenuContext& InContext);
std::string_view GetEditorMenuTextKey(EditorMenuAction InAction);
bool GetToolOfAction(EditorMenuAction InAction, EditorTool& OutTool);
std::string_view GetHistoryTextKey(EditorCommandKind InKind);
} // namespace tanks::model

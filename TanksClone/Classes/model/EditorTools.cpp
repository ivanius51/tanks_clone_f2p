#include "model/EditorTools.h"

namespace tanks::model
{

void EditorToolController::SetTool(const EditorTool InTool)
{
	Tool = InTool;
	bMovePending = false;
}

void EditorToolController::ClearSelection()
{
	bHasSelection = false;
	bMovePending = false;
}

void EditorToolController::Select(const EditorCell& InCell)
{
	bHasSelection = true;
	Selection = InCell;
}

std::unique_ptr<EditorCommand> EditorToolController::Click(const EditorDocument& InDocument, const EditorCell& InCell, const char InPaletteTile)
{
	if ( !InDocument.IsInside(InCell) )
	{
		return nullptr;
	}
	const char Current = InDocument.GetTile(InCell);
	const bool bOccupied = Current != EditorDocument::EmptyTile;

	switch ( Tool )
	{
	case EditorTool::Select:
		if ( bOccupied )
		{
			Select(InCell);
		}
		else
		{
			ClearSelection();
		}
		return nullptr;

	case EditorTool::Place:
		if ( Current == InPaletteTile )
		{
			return nullptr;
		}
		if ( InPaletteTile == EditorDocument::EmptyTile )
		{
			return std::make_unique<RemoveTileCommand>(InCell);
		}
		return std::make_unique<AddTileCommand>(InCell, InPaletteTile);

	case EditorTool::Eraser:
		return bOccupied ? std::make_unique<RemoveTileCommand>(InCell) : nullptr;

	case EditorTool::Move:
		if ( !bMovePending )
		{
			if ( bOccupied )
			{
				Select(InCell);
				bMovePending = true;
			}
			return nullptr;
		}
		bMovePending = false;
		if ( InCell == Selection )
		{
			return nullptr;
		}
		{
			std::unique_ptr<EditorCommand> Move = std::make_unique<MoveTileCommand>(Selection, InCell);
			Select(InCell);
			return Move;
		}
	}
	return nullptr;
}

bool EditorToolController::BeginMove(const EditorDocument& InDocument, const EditorCell& InCell)
{
	if ( !InDocument.IsInside(InCell) || InDocument.GetTile(InCell) == EditorDocument::EmptyTile )
	{
		return false;
	}
	Tool = EditorTool::Move;
	Select(InCell);
	bMovePending = true;
	return true;
}

std::vector<EditorMenuItem> BuildEditorMenu(const EditorMenuContext& InContext)
{
	std::vector<EditorMenuItem> Items;
	const auto Add = [&Items](const EditorMenuAction InAction, const bool bInEnabled, const bool bInChecked, const bool bInSeparatorBefore)
	{
		Items.push_back(EditorMenuItem{ InAction, bInEnabled, bInChecked, bInSeparatorBefore });
	};
	if ( InContext.bOnObject )
	{
		Add(EditorMenuAction::GoToCategory, true, false, false);
		Add(EditorMenuAction::Remove, true, false, false);
		Add(EditorMenuAction::MoveObject, true, false, false);
	}
	const bool bFirstSeparator = InContext.bOnObject;
	Add(EditorMenuAction::ToolSelect, true, InContext.Tool == EditorTool::Select, bFirstSeparator);
	Add(EditorMenuAction::ToolPlace, true, InContext.Tool == EditorTool::Place, false);
	Add(EditorMenuAction::ToolEraser, true, InContext.Tool == EditorTool::Eraser, false);
	Add(EditorMenuAction::ToolMove, true, InContext.Tool == EditorTool::Move, false);
	Add(EditorMenuAction::Undo, InContext.bCanUndo, false, true);
	Add(EditorMenuAction::Redo, InContext.bCanRedo, false, false);
	return Items;
}

std::string_view GetEditorMenuTextKey(const EditorMenuAction InAction)
{
	switch ( InAction )
	{
	case EditorMenuAction::GoToCategory:
		return "EDITOR_GO_CATEGORY";
	case EditorMenuAction::Remove:
		return "EDITOR_REMOVE";
	case EditorMenuAction::MoveObject:
	case EditorMenuAction::ToolMove:
		return "EDITOR_TOOL_MOVE";
	case EditorMenuAction::ToolSelect:
		return "EDITOR_TOOL_SELECT";
	case EditorMenuAction::ToolPlace:
		return "EDITOR_TOOL_PLACE";
	case EditorMenuAction::ToolEraser:
		return "EDITOR_TOOL_ERASER";
	case EditorMenuAction::Undo:
		return "EDITOR_UNDO";
	case EditorMenuAction::Redo:
		return "EDITOR_REDO";
	}
	return "";
}

bool GetToolOfAction(const EditorMenuAction InAction, EditorTool& OutTool)
{
	switch ( InAction )
	{
	case EditorMenuAction::ToolSelect:
		OutTool = EditorTool::Select;
		return true;
	case EditorMenuAction::ToolPlace:
		OutTool = EditorTool::Place;
		return true;
	case EditorMenuAction::ToolEraser:
		OutTool = EditorTool::Eraser;
		return true;
	case EditorMenuAction::ToolMove:
		OutTool = EditorTool::Move;
		return true;
	default:
		return false;
	}
}

std::string_view GetHistoryTextKey(const EditorCommandKind InKind)
{
	switch ( InKind )
	{
	case EditorCommandKind::Add:
		return "EDITOR_HISTORY_ADD";
	case EditorCommandKind::Remove:
		return "EDITOR_HISTORY_REMOVE";
	case EditorCommandKind::Move:
		return "EDITOR_HISTORY_MOVE";
	case EditorCommandKind::Property:
		return "EDITOR_HISTORY_PROPERTY";
	case EditorCommandKind::Script:
		return "EDITOR_HISTORY_SCRIPT";
	}
	return "";
}

} // namespace tanks::model

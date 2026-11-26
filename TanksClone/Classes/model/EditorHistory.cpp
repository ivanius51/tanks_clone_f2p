#include "model/EditorHistory.h"

#include <utility>

namespace game::model
{

void EditorHistory::Execute(EditorDocument& InDocument, TUniquePtr<EditorCommand> InCommand)
{
	Commands.Resize(Cursor);
	InCommand->Apply(InDocument);
	Commands.PushBack(std::move(InCommand));
	Cursor = Commands.Size();
}

bool EditorHistory::Undo(EditorDocument& InDocument)
{
	if ( !CanUndo() )
	{
		return false;
	}
	--Cursor;
	Commands[Cursor]->Revert(InDocument);
	return true;
}

bool EditorHistory::Redo(EditorDocument& InDocument)
{
	if ( !CanRedo() )
	{
		return false;
	}
	Commands[Cursor]->Apply(InDocument);
	++Cursor;
	return true;
}

bool EditorHistory::JumpTo(EditorDocument& InDocument, const std::size_t InStep)
{
	if ( InStep > Commands.Size() )
	{
		return false;
	}
	while ( Cursor > InStep )
	{
		Undo(InDocument);
	}
	while ( Cursor < InStep )
	{
		Redo(InDocument);
	}
	return true;
}

void EditorHistory::Clear()
{
	Commands.Clear();
	Cursor = 0;
}

} // namespace game::model

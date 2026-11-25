#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "model/EditorCommand.h"

namespace game::model
{
/**
 * The undo and redo stacks of the editor, kept as one list with a cursor: the commands before the cursor are applied,
 * the ones after it can be redone. A new command drops the redo part. The history can jump to any step.
 */
class EditorHistory
{
public:
	void Execute(EditorDocument& InDocument, std::unique_ptr<EditorCommand> InCommand);
	bool Undo(EditorDocument& InDocument);
	bool Redo(EditorDocument& InDocument);
	bool JumpTo(EditorDocument& InDocument, std::size_t InStep);
	void Clear();

	bool CanUndo() const { return Cursor > 0; }
	bool CanRedo() const { return Cursor < Commands.size(); }
	std::size_t GetStep() const { return Cursor; }
	std::size_t GetSize() const { return Commands.size(); }
	const EditorCommand& GetCommand(const std::size_t InIndex) const { return *Commands[InIndex]; }

private:
	std::vector<std::unique_ptr<EditorCommand>> Commands;
	std::size_t Cursor = 0;
};
} // namespace game::model

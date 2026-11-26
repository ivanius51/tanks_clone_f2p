#pragma once

#include <cstddef>
#include <memory>

#include "model/EditorCommand.h"
#include "templates/TUniquePtr.h"
#include "templates/TContainers.h"

namespace game::model
{
/**
 * The undo and redo stacks of the editor, kept as one list with a cursor: the commands before the cursor are applied,
 * the ones after it can be redone. A new command drops the redo part. The history can jump to any step.
 */
class EditorHistory
{
public:
	EditorHistory() = default;
	EditorHistory(const EditorHistory&) = delete;
	EditorHistory& operator=(const EditorHistory&) = delete;
	EditorHistory(EditorHistory&&) noexcept = default;
	EditorHistory& operator=(EditorHistory&&) noexcept = default;

	void Execute(EditorDocument& InDocument, TUniquePtr<EditorCommand> InCommand);
	bool Undo(EditorDocument& InDocument);
	bool Redo(EditorDocument& InDocument);
	bool JumpTo(EditorDocument& InDocument, std::size_t InStep);
	void Clear();

	bool CanUndo() const { return Cursor > 0; }
	bool CanRedo() const { return Cursor < Commands.Size(); }
	std::size_t GetStep() const { return Cursor; }
	std::size_t GetSize() const { return Commands.Size(); }
	const EditorCommand& GetCommand(const std::size_t InIndex) const { return *Commands[InIndex]; }

private:
	TArray<TUniquePtr<EditorCommand>> Commands;
	std::size_t Cursor = 0;
};
} // namespace game::model

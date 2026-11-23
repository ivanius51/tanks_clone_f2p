#include "model/EditorCommand.h"

#include <utility>

namespace tanks::model
{

namespace
{
	std::string Describe(const EditorCell& InCell)
	{
		return "(" + std::to_string(InCell.Column) + "," + std::to_string(InCell.Row) + ")";
	}
}

AddTileCommand::AddTileCommand(const EditorCell& InCell, const char InTile)
	: Cell(InCell)
	, Tile(InTile)
{
}

void AddTileCommand::Apply(EditorDocument& InDocument)
{
	ReplacedTile = InDocument.GetTile(Cell);
	bMovedUnique = false;
	if ( InDocument.IsUnique(Tile) && InDocument.FindTile(Tile, UniqueFrom) )
	{
		bMovedUnique = true;
		InDocument.SetTile(UniqueFrom, EditorDocument::EmptyTile);
	}
	InDocument.SetTile(Cell, Tile);
}

void AddTileCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetTile(Cell, ReplacedTile);
	if ( bMovedUnique )
	{
		InDocument.SetTile(UniqueFrom, Tile);
	}
}

std::string AddTileCommand::GetDetail() const
{
	return std::string(1, Tile) + " " + Describe(Cell);
}

RemoveTileCommand::RemoveTileCommand(const EditorCell& InCell)
	: Cell(InCell)
{
}

void RemoveTileCommand::Apply(EditorDocument& InDocument)
{
	RemovedTile = InDocument.GetTile(Cell);
	InDocument.SetTile(Cell, EditorDocument::EmptyTile);
}

void RemoveTileCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetTile(Cell, RemovedTile);
}

std::string RemoveTileCommand::GetDetail() const
{
	return Describe(Cell);
}

MoveTileCommand::MoveTileCommand(const EditorCell& InFrom, const EditorCell& InTo)
	: From(InFrom)
	, To(InTo)
{
}

void MoveTileCommand::Apply(EditorDocument& InDocument)
{
	MovedTile = InDocument.GetTile(From);
	ReplacedTile = InDocument.GetTile(To);
	if ( From == To )
	{
		return;
	}
	InDocument.SetTile(From, EditorDocument::EmptyTile);
	InDocument.SetTile(To, MovedTile);
}

void MoveTileCommand::Revert(EditorDocument& InDocument)
{
	if ( From == To )
	{
		return;
	}
	InDocument.SetTile(To, ReplacedTile);
	InDocument.SetTile(From, MovedTile);
}

std::string MoveTileCommand::GetDetail() const
{
	return std::string(1, MovedTile) + " " + Describe(From) + " > " + Describe(To);
}

SetPropertyCommand::SetPropertyCommand(std::string InKey, std::string InValue)
	: Key(std::move(InKey))
	, NewValue(std::move(InValue))
{
}

void SetPropertyCommand::Apply(EditorDocument& InDocument)
{
	OldValue = InDocument.GetProperty(Key);
	InDocument.SetProperty(Key, NewValue);
}

void SetPropertyCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetProperty(Key, OldValue);
}

SetScriptCommand::SetScriptCommand(std::string InText)
	: NewText(std::move(InText))
{
}

void SetScriptCommand::Apply(EditorDocument& InDocument)
{
	OldText = InDocument.GetScriptText();
	InDocument.SetScriptText(NewText);
}

void SetScriptCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetScriptText(OldText);
}

} // namespace tanks::model

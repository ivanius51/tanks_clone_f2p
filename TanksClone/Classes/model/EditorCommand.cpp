#include "model/EditorCommand.h"

#include <utility>

namespace game::model
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

ResizeMapCommand::ResizeMapCommand(const int InRows, const int InColumns, const ResizeAnchor InAnchor)
	: Rows(InRows)
	, Columns(InColumns)
	, Anchor(InAnchor)
{
}

void ResizeMapCommand::Apply(EditorDocument& InDocument)
{
	OldRows = InDocument.GetRows();
	OldFloor = InDocument.GetFloor();
	OldLayers = InDocument.GetLayers();
	InDocument.Resize(Rows, Columns, Anchor);
}

void ResizeMapCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetRows(OldRows);
	InDocument.SetFloor(OldFloor);
	InDocument.SetLayers(OldLayers);
}

std::string ResizeMapCommand::GetDetail() const
{
	return std::to_string(Columns) + "x" + std::to_string(Rows);
}

SetFloorCommand::SetFloorCommand(const EditorCell& InCell, const char InFloor)
	: Cell(InCell)
	, Floor(InFloor)
{
}

void SetFloorCommand::Apply(EditorDocument& InDocument)
{
	OldFloor = InDocument.GetFloor();
	InDocument.SetFloorTile(Cell, Floor);
}

void SetFloorCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetFloor(OldFloor);
}

std::string SetFloorCommand::GetDetail() const
{
	return std::string(1, Floor) + " " + std::to_string(Cell.Column) + "," + std::to_string(Cell.Row);
}

SetPointCommand::SetPointCommand(const EditorCell& InCell, std::string InName)
	: Cell(InCell)
	, Name(std::move(InName))
{
}

void SetPointCommand::Apply(EditorDocument& InDocument)
{
	OldLayers = InDocument.GetLayers();
	LevelLayers Layers = OldLayers;
	const LevelPoint* const Existing = Layers.FindPointAt(Cell.Column, Cell.Row);
	if ( Existing && Existing->Name != Name )
	{
		Layers.RemovePoint(std::string(Existing->Name));
	}
	if ( !Name.empty() )
	{
		Layers.SetPoint(Name, Cell.Column, Cell.Row);
	}
	InDocument.SetLayers(std::move(Layers));
}

void SetPointCommand::Revert(EditorDocument& InDocument)
{
	InDocument.SetLayers(OldLayers);
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

} // namespace game::model

#pragma once

#include <string>

#include "model/EditorDocument.h"
#include "templates/TContainers.h"

namespace game::model
{
enum class EditorCommandKind
{
	Add,
	Remove,
	Move,
	Property,
	Script,
	Resize,
	Floor,
	Points
};

/**
 * One undoable editor action. Apply may be called again after Revert (redo) and must give the same result;
 * Revert restores exactly what Apply changed.
 */
class EditorCommand
{
public:
	virtual ~EditorCommand() = default;

	virtual void Apply(EditorDocument& InDocument) = 0;
	virtual void Revert(EditorDocument& InDocument) = 0;
	virtual EditorCommandKind GetKind() const = 0;
	virtual std::string GetDetail() const = 0;
};

/** Puts a tile on a cell. A unique tile (the player start) first leaves its old cell. */
class AddTileCommand : public EditorCommand
{
public:
	AddTileCommand(const EditorCell& InCell, char InTile);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Add; }
	std::string GetDetail() const override;

private:
	EditorCell Cell;
	char Tile;
	char ReplacedTile = EditorDocument::EmptyTile;
	bool bMovedUnique = false;
	EditorCell UniqueFrom;
};

/** Clears a cell. */
class RemoveTileCommand : public EditorCommand
{
public:
	explicit RemoveTileCommand(const EditorCell& InCell);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Remove; }
	std::string GetDetail() const override;

private:
	EditorCell Cell;
	char RemovedTile = EditorDocument::EmptyTile;
};

/** Takes the tile from one cell and puts it on another; one undo step. */
class MoveTileCommand : public EditorCommand
{
public:
	MoveTileCommand(const EditorCell& InFrom, const EditorCell& InTo);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Move; }
	std::string GetDetail() const override;

private:
	EditorCell From;
	EditorCell To;
	char MovedTile = EditorDocument::EmptyTile;
	char ReplacedTile = EditorDocument::EmptyTile;
};

/** Changes one level property (name, music, tileset, enemy count). */
class SetPropertyCommand : public EditorCommand
{
public:
	SetPropertyCommand(std::string InKey, std::string InValue);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Property; }
	std::string GetDetail() const override { return Key; }

private:
	std::string Key;
	std::string NewValue;
	std::string OldValue;
};

/** Changes the map's size; a snapshot of the old rows undoes it, so a shrink that cut tiles gets them back. */
class ResizeMapCommand : public EditorCommand
{
public:
	ResizeMapCommand(int InRows, int InColumns, ResizeAnchor InAnchor);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Resize; }
	std::string GetDetail() const override;

private:
	int Rows;
	int Columns;
	ResizeAnchor Anchor;
	TArray<std::string> OldRows;
	TArray<std::string> OldFloor;
	LevelLayers OldLayers;
};

/** Paints one cell of the floor (a ground modifier: water, road...); '.' clears it. */
class SetFloorCommand : public EditorCommand
{
public:
	SetFloorCommand(const EditorCell& InCell, char InFloor);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Floor; }
	std::string GetDetail() const override;

private:
	EditorCell Cell;
	char Floor;
	TArray<std::string> OldFloor;
};

/** Places, moves or removes a named point; the old layers are the undo snapshot. */
class SetPointCommand : public EditorCommand
{
public:
	// An empty name removes the point at the cell.
	SetPointCommand(const EditorCell& InCell, std::string InName);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Points; }
	std::string GetDetail() const override { return Name; }

private:
	EditorCell Cell;
	std::string Name;
	LevelLayers OldLayers;
};

/** A snapshot of the whole script text: any script edit is one history entry. */
class SetScriptCommand : public EditorCommand
{
public:
	explicit SetScriptCommand(std::string InText);

	void Apply(EditorDocument& InDocument) override;
	void Revert(EditorDocument& InDocument) override;
	EditorCommandKind GetKind() const override { return EditorCommandKind::Script; }
	std::string GetDetail() const override { return std::string(); }

private:
	std::string NewText;
	std::string OldText;
};
} // namespace game::model

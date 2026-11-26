#include "ui/TileMergeView.h"

#include <algorithm>

#include "core/GameConstants.h"
#include "templates/TUniquePtr.h"

namespace game::view::cocos
{

TileMergeView* TileMergeView::Create(const model::TileMergeSettings& InSettings, const int InColumns, const int InRows, const unsigned InSeed)
{
	TileMergeView* const View = new TileMergeView(InSettings, InColumns, InRows, InSeed);
	if ( !View->init() )
	{
		delete View;
		return nullptr;
	}
	View->autorelease();
	View->scheduleUpdate();
	return View;
}

TileMergeView::TileMergeView(const model::TileMergeSettings& InSettings, const int InColumns, const int InRows, const unsigned InSeed)
	: Settings(InSettings)
{
	Field.Resize(InColumns, InRows);
	Baker = MakeUnique<model::TileMergeBaker>(Settings, InSeed);
	const int ChunkCells = Settings.GetChunkCells();
	ChunkColumns = (InColumns + ChunkCells - 1) / ChunkCells;
	ChunkRows = (InRows + ChunkCells - 1) / ChunkCells;
	Chunks.Reserve(static_cast<size_t>(ChunkColumns) * static_cast<size_t>(ChunkRows));
	for ( int ChunkRow = 0; ChunkRow < ChunkRows; ++ChunkRow )
	{
		for ( int ChunkColumn = 0; ChunkColumn < ChunkColumns; ++ChunkColumn )
		{
			Chunk NewChunk;
			NewChunk.Area.FirstColumn = ChunkColumn * ChunkCells;
			NewChunk.Area.FirstRow = ChunkRow * ChunkCells;
			NewChunk.Area.Columns = std::min(ChunkCells, InColumns - NewChunk.Area.FirstColumn);
			NewChunk.Area.Rows = std::min(ChunkCells, InRows - NewChunk.Area.FirstRow);
			NewChunk.Area.CellPixels = Settings.GetCellPixels();
			Chunks.PushBack(NewChunk);
		}
	}
}

TileMergeView::Chunk* TileMergeView::FindChunk(const int InColumn, const int InRow)
{
	if ( InColumn < 0 || InRow < 0 || InColumn >= Field.GetColumns() || InRow >= Field.GetRows() )
	{
		return nullptr;
	}
	const int ChunkCells = Settings.GetChunkCells();
	return &Chunks[static_cast<size_t>(InRow / ChunkCells) * static_cast<size_t>(ChunkColumns) + static_cast<size_t>(InColumn / ChunkCells)];
}

void TileMergeView::MarkDirty(const int InColumn, const int InRow)
{
	for ( int RowShift = -1; RowShift <= 1; ++RowShift )
	{
		for ( int ColumnShift = -1; ColumnShift <= 1; ++ColumnShift )
		{
			if ( Chunk* const Near = FindChunk(InColumn + ColumnShift, InRow + RowShift) )
			{
				Near->bDirty = true;
			}
		}
	}
}

void TileMergeView::SetCell(const int InColumn, const int InRow, const model::MergeMaterial InMaterial, const float InAmount, const bool bInInstant)
{
	if ( bInInstant )
	{
		Field.SetCellInstant(InColumn, InRow, InMaterial, InAmount);
	}
	else
	{
		Field.SetCell(InColumn, InRow, InMaterial, InAmount);
	}
	MarkDirty(InColumn, InRow);
}

bool TileMergeView::ToCell(const cocos2d::Vec2& InPosition, int& OutColumn, int& OutRow) const
{
	OutColumn = static_cast<int>(std::floor(InPosition.x / TileSize));
	OutRow = Field.GetRows() - 1 - static_cast<int>(std::floor(InPosition.y / TileSize));
	return OutColumn >= 0 && OutRow >= 0 && OutColumn < Field.GetColumns() && OutRow < Field.GetRows();
}

void TileMergeView::BakeAll()
{
	for ( Chunk& Current : Chunks )
	{
		Current.bDirty = true;
	}
}

void TileMergeView::update(const float InDelta)
{
	cocos2d::Node::update(InDelta);
	if ( Field.Step(InDelta, Settings.GetFadeTime()) )
	{
		for ( const int Cell : Field.GetChangedCells() )
		{
			MarkDirty(Cell % Field.GetColumns(), Cell / Field.GetColumns());
		}
	}
	for ( Chunk& Current : Chunks )
	{
		if ( Current.bDirty )
		{
			BakeChunk(Current);
		}
	}
}

void TileMergeView::BakeChunk(Chunk& InChunk)
{
	InChunk.bDirty = false;
	Scratch.Resize(InChunk.Area.GetByteCount());
	Baker->Bake(Field, InChunk.Area, Scratch.GetData());
	const int Width = InChunk.Area.GetWidth();
	const int Height = InChunk.Area.GetHeight();
	if ( !InChunk.Texture )
	{
		InChunk.Texture = new cocos2d::Texture2D();
		InChunk.Texture->initWithData(Scratch.GetData(), static_cast<ssize_t>(Scratch.Size()), cocos2d::Texture2D::PixelFormat::RGBA8888,
			Width, Height, cocos2d::Size(static_cast<float>(Width), static_cast<float>(Height)));
		InChunk.Texture->autorelease();
		InChunk.Image = cocos2d::Sprite::createWithTexture(InChunk.Texture);
		InChunk.Image->setAnchorPoint(cocos2d::Vec2::ZERO);
		InChunk.Image->setScale(TileSize / static_cast<float>(InChunk.Area.CellPixels));
		const float Top = static_cast<float>(Field.GetRows() - InChunk.Area.FirstRow - InChunk.Area.Rows) * TileSize;
		InChunk.Image->setPosition(static_cast<float>(InChunk.Area.FirstColumn) * TileSize, Top);
		addChild(InChunk.Image);
		return;
	}
	InChunk.Texture->updateWithData(Scratch.GetData(), 0, 0, Width, Height);
}

} // namespace game::view::cocos

#include "ui/TileMergeView.h"

#include <algorithm>

#include "core/GameConstants.h"
#include "templates/TUniquePtr.h"

namespace game::view::cocos
{
namespace
{
constexpr int ChunkPadPixels = 1;
constexpr int ShadowReachCells = 3;
}

TileMergeView* TileMergeView::Create(const model::TileMergeSettings& InSettings, const int InColumns, const int InRows, const unsigned InSeed, const int InSubdivision)
{
	TileMergeView* const View = new TileMergeView(InSettings, InColumns, InRows, InSeed, InSubdivision);
	if ( !View->init() )
	{
		delete View;
		return nullptr;
	}
	View->autorelease();
	View->scheduleUpdate();
	return View;
}

TileMergeView::TileMergeView(const model::TileMergeSettings& InSettings, const int InColumns, const int InRows, const unsigned InSeed, const int InSubdivision)
	: Settings(InSettings)
	, Subdivision(std::max(InSubdivision, 1))
{
	const int FieldColumns = InColumns * Subdivision;
	const int FieldRows = InRows * Subdivision;
	Field.Resize(FieldColumns, FieldRows);
	Baker = MakeUnique<model::TileMergeBaker>(Settings, InSeed, Subdivision);
	const int ChunkCells = Settings.GetChunkCells() * Subdivision;
	ChunkColumns = (FieldColumns + ChunkCells - 1) / ChunkCells;
	ChunkRows = (FieldRows + ChunkCells - 1) / ChunkCells;
	Chunks.Reserve(static_cast<size_t>(ChunkColumns) * static_cast<size_t>(ChunkRows));
	for ( int ChunkRow = 0; ChunkRow < ChunkRows; ++ChunkRow )
	{
		for ( int ChunkColumn = 0; ChunkColumn < ChunkColumns; ++ChunkColumn )
		{
			Chunk NewChunk;
			NewChunk.Area.FirstColumn = ChunkColumn * ChunkCells;
			NewChunk.Area.FirstRow = ChunkRow * ChunkCells;
			NewChunk.Area.Columns = std::min(ChunkCells, FieldColumns - NewChunk.Area.FirstColumn);
			NewChunk.Area.Rows = std::min(ChunkCells, FieldRows - NewChunk.Area.FirstRow);
			NewChunk.Area.CellPixels = std::max(Settings.GetCellPixels() / Subdivision, 1);
				NewChunk.Area.PadPixels = ChunkPadPixels;
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
	const int ChunkCells = Settings.GetChunkCells() * Subdivision;
	return &Chunks[static_cast<size_t>(InRow / ChunkCells) * static_cast<size_t>(ChunkColumns) + static_cast<size_t>(InColumn / ChunkCells)];
}

void TileMergeView::MarkDirty(const int InColumn, const int InRow)
{
	const int Reach = ShadowParent ? ShadowReachCells : 1;
	for ( int RowShift = -Reach; RowShift <= Reach; ++RowShift )
	{
		for ( int ColumnShift = -Reach; ColumnShift <= Reach; ++ColumnShift )
		{
			if ( Chunk* const Near = FindChunk(InColumn + ColumnShift, InRow + RowShift) )
			{
				Near->bDirty = true;
			}
		}
	}
}

void TileMergeView::SetCell(const int InColumn, const int InRow, const model::MergeMaterial InMaterial, const float InAmount)
{
	if ( Settings.GetFadeTime() > 0.0f )
	{
		Field.SetCell(InColumn, InRow, InMaterial, InAmount);
	}
	else
	{
		Field.SetCellInstant(InColumn, InRow, InMaterial, InAmount);
	}
	MarkDirty(InColumn, InRow);
}

bool TileMergeView::ToCell(const cocos2d::Vec2& InPosition, int& OutColumn, int& OutRow) const
{
	OutColumn = static_cast<int>(std::floor(InPosition.x / TileSize));
	OutRow = Field.GetRows() / Subdivision - 1 - static_cast<int>(std::floor(InPosition.y / TileSize));
	return OutColumn >= 0 && OutRow >= 0 && OutColumn < Field.GetColumns() / Subdivision && OutRow < Field.GetRows() / Subdivision;
}

void TileMergeView::BakeAll()
{
	for ( Chunk& Current : Chunks )
	{
		Current.bDirty = true;
	}
}

void TileMergeView::Flush()
{
	BakeSome(static_cast<int>(Chunks.Size()));
}

int TileMergeView::BakeSome(int InBudget)
{
	int Remaining = 0;
	for ( Chunk& Current : Chunks )
	{
		if ( !Current.bDirty )
		{
			continue;
		}
		if ( InBudget > 0 )
		{
			BakeChunk(Current);
			--InBudget;
		}
		else
		{
			++Remaining;
		}
	}
	return Remaining;
}

int TileMergeView::CountDirty() const
{
	int Count = 0;
	for ( const Chunk& Current : Chunks )
	{
		Count += Current.bDirty ? 1 : 0;
	}
	return Count;
}

void TileMergeView::SetShadowParent(cocos2d::Node* const InParent)
{
	ShadowParent = InParent;
}

void TileMergeView::ApplySettings(const model::TileMergeSettings& InSettings)
{
	Settings = InSettings;
	BakeAll();
}

void TileMergeView::update(const float InDelta)
{
	cocos2d::Node::update(InDelta);
	if ( Settings.GetFadeTime() > 0.0f && Field.Step(InDelta, Settings.GetFadeTime()) )
	{
		for ( const int Cell : Field.GetChangedCells() )
		{
			MarkDirty(Cell % Field.GetColumns(), Cell / Field.GetColumns());
		}
	}
	int Budget = Settings.GetBakeBudget();
	for ( Chunk& Current : Chunks )
	{
		if ( Current.bDirty && Budget > 0 )
		{
			BakeChunk(Current);
			--Budget;
		}
	}
}

void TileMergeView::UploadLayer(const Chunk& InChunk, cocos2d::Node* const InParent, cocos2d::Texture2D*& InOutTexture, cocos2d::Sprite*& InOutSprite)
{
	const int Width = InChunk.Area.GetWidth();
	const int Height = InChunk.Area.GetHeight();
	if ( InOutTexture )
	{
		InOutTexture->updateWithData(Scratch.GetData(), 0, 0, Width, Height);
		return;
	}
	InOutTexture = new cocos2d::Texture2D();
	InOutTexture->initWithData(Scratch.GetData(), static_cast<ssize_t>(Scratch.Size()), cocos2d::Texture2D::PixelFormat::RGBA8888,
		Width, Height, cocos2d::Size(static_cast<float>(Width), static_cast<float>(Height)));
	InOutTexture->autorelease();
	InOutSprite = cocos2d::Sprite::createWithTexture(InOutTexture);
	InOutSprite->setAnchorPoint(cocos2d::Vec2::ZERO);
	// getContentSize() is in points: the texture size divided by the content scale factor (2 with HD art).
	const cocos2d::Size Points = InOutSprite->getContentSize();
	const float FieldCellSize = TileSize / static_cast<float>(Subdivision);
	const float PixelSize = FieldCellSize / static_cast<float>(InChunk.Area.CellPixels);
	const float Pad = static_cast<float>(InChunk.Area.PadPixels) * PixelSize;
	InOutSprite->setScale((static_cast<float>(InChunk.Area.Columns) * FieldCellSize + 2.0f * Pad) / Points.width, (static_cast<float>(InChunk.Area.Rows) * FieldCellSize + 2.0f * Pad) / Points.height);
	InOutSprite->setPosition(static_cast<float>(InChunk.Area.FirstColumn) * FieldCellSize - Pad, static_cast<float>(Field.GetRows() - InChunk.Area.FirstRow - InChunk.Area.Rows) * FieldCellSize - Pad);
	InParent->addChild(InOutSprite);
}

void TileMergeView::BakeChunk(Chunk& InChunk)
{
	InChunk.bDirty = false;
	Scratch.Resize(InChunk.Area.GetByteCount());
	Baker->Bake(Field, InChunk.Area, Scratch.GetData());
	UploadLayer(InChunk, this, InChunk.Texture, InChunk.Image);
	if ( !ShadowParent || !Settings.IsShadowEnabled() )
	{
		return;
	}
	Chunk Flat = InChunk;
	Flat.Area.PadPixels = 0;
	Scratch.Resize(Flat.Area.GetByteCount());
	Baker->BakeShadow(Field, Flat.Area, Scratch.GetData());
	UploadLayer(Flat, ShadowParent, InChunk.ShadowTexture, InChunk.ShadowImage);
}

} // namespace game::view::cocos

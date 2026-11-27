#include "ui/TileMergeView.h"

#include <algorithm>
#include <cmath>
#include <thread>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "model/Profiler.h"
#include "model/TileMergeBaker.h"
#include "model/TileMergeBakerCached.h"
#include "services/ProfileManager.h"

namespace game::view::cocos
{
namespace
{
constexpr int ChunkPadPixels = 1;
// The baker's texture coordinates are 64 units per tile; shadow, face and edge light reach a number of them.
constexpr float TextureUnitsPerTile = 64.0f;
constexpr float EdgeLightPixels = 2.0f;
// Jitter of the merged outline and the rounding of corners reach about a cell beyond the changed one.
constexpr int ReachMarginCells = 2;
constexpr int MaxWorkers = 4;
constexpr std::chrono::microseconds UploadBudget{ 4000 };
}

void TileMergeView::CellRect::Add(const CellRect& InOther)
{
	if ( InOther.IsEmpty() )
	{
		return;
	}
	if ( IsEmpty() )
	{
		*this = InOther;
		return;
	}
	FirstColumn = std::min(FirstColumn, InOther.FirstColumn);
	FirstRow = std::min(FirstRow, InOther.FirstRow);
	EndColumn = std::max(EndColumn, InOther.EndColumn);
	EndRow = std::max(EndRow, InOther.EndRow);
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
	, Seed(InSeed)
	, bSmooth(GameContext::Get<ProfileManager>().Get().bSmoothWalls)
{
	const int FieldColumns = InColumns * Subdivision;
	const int FieldRows = InRows * Subdivision;
	Field.Resize(FieldColumns, FieldRows);
	BuildBackend();
}

void TileMergeView::BuildBackend()
{
	const int FieldColumns = Field.GetColumns();
	const int FieldRows = Field.GetRows();
	const model::MergeBakeBackend Backend = Settings.GetBakeBackend();
	if ( Backend == model::MergeBakeBackend::Gpu )
	{
		Gpu.Reset(TileMergeGpu::Create(Settings, Seed, Subdivision, FieldColumns, FieldRows, this));
	}
	if ( Gpu )
	{
		bGpuDirty = true;
		return;
	}
	if ( Backend == model::MergeBakeBackend::Reference )
	{
		Baker = MakeUnique<model::TileMergeBaker>(Settings, Seed, Subdivision);
	}
	else
	{
		Baker = MakeUnique<model::TileMergeBakerCached>(Settings, Seed, Subdivision);
	}
	ReachCells = ComputeReach();
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
	const int WorkerCount = std::clamp(static_cast<int>(std::thread::hardware_concurrency()) - 1, 1, MaxWorkers);
	for ( int Index = 0; Index < WorkerCount; ++Index )
	{
		Workers.PushBack(MakeUnique<model::WorkQueue>());
	}
}

TileMergeView::~TileMergeView()
{
	Workers.Clear();
}

void TileMergeView::ClearBackend()
{
	DrainWorkers();
	Workers.Clear();
	for ( Chunk& Current : Chunks )
	{
		for ( cocos2d::Sprite* const Image : { Current.Image, Current.ShadowImage } )
		{
			if ( Image )
			{
				Image->removeFromParent();
			}
		}
	}
	Chunks.Clear();
	Baker.Reset();
	Gpu.Reset();
	InFlightCount = 0;
	NextWorker = 0;
	bGpuDirty = false;
}

void TileMergeView::Reconfigure(const model::TileMergeSettings& InSettings)
{
	ClearBackend();
	Settings = InSettings;
	BuildBackend();
	if ( Gpu )
	{
		Gpu->SetShadowParent(ShadowParent);
	}
	ReachCells = ComputeReach();
	BakeAll();
}

const char* TileMergeView::GetBackendName() const
{
	return Gpu ? "gpu" : Baker->GetName();
}

int TileMergeView::ComputeReach() const
{
	float Pixels = std::max(Settings.GetSideFacePx(), EdgeLightPixels);
	if ( ShadowParent && Settings.IsShadowEnabled() )
	{
		Pixels = std::max({ Pixels, Settings.GetShadowOffsetPx(), Settings.GetShadowLengthPx() });
	}
	return static_cast<int>(std::ceil(Pixels / TextureUnitsPerTile * static_cast<float>(Subdivision))) + ReachMarginCells;
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
	if ( Gpu )
	{
		bGpuDirty = true;
		return;
	}
	const int ChunkCells = Settings.GetChunkCells() * Subdivision;
	const int FirstColumn = std::max(InColumn - ReachCells, 0);
	const int FirstRow = std::max(InRow - ReachCells, 0);
	const int LastColumn = std::min(InColumn + ReachCells, Field.GetColumns() - 1);
	const int LastRow = std::min(InRow + ReachCells, Field.GetRows() - 1);
	for ( int ChunkRow = FirstRow / ChunkCells; ChunkRow <= LastRow / ChunkCells; ++ChunkRow )
	{
		for ( int ChunkColumn = FirstColumn / ChunkCells; ChunkColumn <= LastColumn / ChunkCells; ++ChunkColumn )
		{
			Chunk& Near = Chunks[static_cast<size_t>(ChunkRow) * static_cast<size_t>(ChunkColumns) + static_cast<size_t>(ChunkColumn)];
			CellRect Touched;
			Touched.FirstColumn = std::max(FirstColumn, Near.Area.FirstColumn);
			Touched.FirstRow = std::max(FirstRow, Near.Area.FirstRow);
			Touched.EndColumn = std::min(LastColumn + 1, Near.Area.FirstColumn + Near.Area.Columns);
			Touched.EndRow = std::min(LastRow + 1, Near.Area.FirstRow + Near.Area.Rows);
			Near.Dirty.Add(Touched);
		}
	}
}

void TileMergeView::MarkWholeDirty(Chunk& InChunk)
{
	InChunk.Dirty = { InChunk.Area.FirstColumn, InChunk.Area.FirstRow, InChunk.Area.FirstColumn + InChunk.Area.Columns, InChunk.Area.FirstRow + InChunk.Area.Rows };
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
	bGpuDirty = Gpu ? true : bGpuDirty;
	for ( Chunk& Current : Chunks )
	{
		MarkWholeDirty(Current);
	}
}

void TileMergeView::SetShadowParent(cocos2d::Node* const InParent)
{
	ShadowParent = InParent;
	if ( Gpu )
	{
		Gpu->SetShadowParent(InParent);
	}
	ReachCells = ComputeReach();
}

void TileMergeView::ApplySettings(const model::TileMergeSettings& InSettings)
{
	DrainWorkers();
	Settings = InSettings;
	ReachCells = ComputeReach();
	if ( Gpu )
	{
		Gpu->ApplySettings(Settings);
	}
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
	RefreshFilter();
	Advance();
	DrainWorkers();
}

void TileMergeView::ApplyFilter(cocos2d::Texture2D* const InTexture) const
{
	if ( bSmooth && Settings.HasMipmaps() )
	{
		InTexture->generateMipmap();
		InTexture->setTexParameters({ GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE });
		return;
	}
	if ( bSmooth )
	{
		InTexture->setAntiAliasTexParameters();
		return;
	}
	InTexture->setAliasTexParameters();
}

void TileMergeView::RefreshFilter()
{
	const bool bWanted = GameContext::Get<ProfileManager>().Get().bSmoothWalls;
	if ( bWanted == bSmooth )
	{
		return;
	}
	bSmooth = bWanted;
	for ( const Chunk& Current : Chunks )
	{
		for ( cocos2d::Texture2D* const Texture : { Current.Texture, Current.ShadowTexture } )
		{
			if ( Texture )
			{
				ApplyFilter(Texture);
			}
		}
	}
}

void TileMergeView::UploadGpu()
{
	if ( !Gpu || !bGpuDirty )
	{
		return;
	}
	const model::ProfileScope Scope("TileMerge.GpuUpload");
	Gpu->Upload(Field);
	bGpuDirty = false;
}

int TileMergeView::CountPending() const
{
	int Count = 0;
	for ( const Chunk& Current : Chunks )
	{
		Count += !Current.Dirty.IsEmpty() || Current.bInFlight ? 1 : 0;
	}
	return Count;
}

TileMergeView::CellRect TileMergeView::TakeDirty(Chunk& InChunk) const
{
	if ( !InChunk.Texture )
	{
		// The first bake creates the textures, so it covers the whole chunk.
		const CellRect Whole{ InChunk.Area.FirstColumn, InChunk.Area.FirstRow, InChunk.Area.FirstColumn + InChunk.Area.Columns, InChunk.Area.FirstRow + InChunk.Area.Rows };
		InChunk.Dirty = {};
		return Whole;
	}
	const CellRect Taken = InChunk.Dirty;
	InChunk.Dirty = {};
	return Taken;
}

TileMergeView::BakeResult TileMergeView::BakeRegion(const model::TileMergeField& InField, const Chunk& InChunk, const CellRect& InRegion, const bool bInShadow) const
{
	BakeResult Result;
	Result.Region = InRegion;
	model::MergeChunk Area = InChunk.Area;
	Area.FirstColumn = InRegion.FirstColumn;
	Area.FirstRow = InRegion.FirstRow;
	Area.Columns = InRegion.EndColumn - InRegion.FirstColumn;
	Area.Rows = InRegion.EndRow - InRegion.FirstRow;
	Area.PadPixels = ChunkPadPixels;
	Result.Color.Resize(Area.GetByteCount());
	{
		const model::ProfileScope Scope("TileMerge.BakeColor");
		Baker->Bake(InField, Area, Result.Color.GetData());
	}
	if ( bInShadow )
	{
		Area.PadPixels = 0;
		Result.Shadow.Resize(Area.GetByteCount());
		const model::ProfileScope Scope("TileMerge.BakeShadow");
		Baker->BakeShadow(InField, Area, Result.Shadow.GetData());
	}
	return Result;
}

namespace
{
int NextPowerOfTwo(const int InValue)
{
	int Result = 1;
	while ( Result < InValue )
	{
		Result *= 2;
	}
	return Result;
}
}

void TileMergeView::UploadLayer(const Chunk& InChunk, const CellRect& InRegion, const TArray<std::uint8_t>& InPixels, const int InPad, cocos2d::Node* const InParent, cocos2d::Texture2D*& InOutTexture, cocos2d::Sprite*& InOutSprite)
{
	const int CellPixels = InChunk.Area.CellPixels;
	const int Width = (InRegion.EndColumn - InRegion.FirstColumn) * CellPixels + 2 * InPad;
	const int Height = (InRegion.EndRow - InRegion.FirstRow) * CellPixels + 2 * InPad;
	if ( InOutTexture )
	{
		InOutTexture->updateWithData(InPixels.GetData(), (InRegion.FirstColumn - InChunk.Area.FirstColumn) * CellPixels, (InRegion.FirstRow - InChunk.Area.FirstRow) * CellPixels, Width, Height);
		if ( Settings.HasMipmaps() && bSmooth )
		{
			InOutTexture->generateMipmap();
		}
		return;
	}
	// Mipmaps need power-of-two sizes (ES2, WebGL): the chunk sits in the top-left corner of a larger texture and the sprite shows only that rectangle.
	const int PotWidth = NextPowerOfTwo(Width);
	const int PotHeight = NextPowerOfTwo(Height);
	TArray<std::uint8_t> Padded(static_cast<size_t>(PotWidth) * static_cast<size_t>(PotHeight) * 4);
	std::fill(Padded.GetData(), Padded.GetData() + Padded.Size(), static_cast<std::uint8_t>(0));
	for ( int Row = 0; Row < Height; ++Row )
	{
		std::copy_n(InPixels.GetData() + static_cast<size_t>(Row) * static_cast<size_t>(Width) * 4, static_cast<size_t>(Width) * 4, Padded.GetData() + static_cast<size_t>(Row) * static_cast<size_t>(PotWidth) * 4);
	}
	InOutTexture = new cocos2d::Texture2D();
	InOutTexture->initWithData(Padded.GetData(), static_cast<ssize_t>(Padded.Size()), cocos2d::Texture2D::PixelFormat::RGBA8888,
		PotWidth, PotHeight, cocos2d::Size(static_cast<float>(PotWidth), static_cast<float>(PotHeight)));
	InOutTexture->autorelease();
	ApplyFilter(InOutTexture);
	InOutSprite = cocos2d::Sprite::createWithTexture(InOutTexture, CC_RECT_PIXELS_TO_POINTS(cocos2d::Rect(0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height))));
	InOutSprite->setAnchorPoint(cocos2d::Vec2::ZERO);
	// getContentSize() is in points: the texture size divided by the content scale factor (2 with HD art).
	const cocos2d::Size Points = InOutSprite->getContentSize();
	const float FieldCellSize = TileSize / static_cast<float>(Subdivision);
	const float PixelSize = FieldCellSize / static_cast<float>(CellPixels);
	const float Pad = static_cast<float>(InPad) * PixelSize;
	InOutSprite->setScale((static_cast<float>(InChunk.Area.Columns) * FieldCellSize + 2.0f * Pad) / Points.width, (static_cast<float>(InChunk.Area.Rows) * FieldCellSize + 2.0f * Pad) / Points.height);
	InOutSprite->setPosition(static_cast<float>(InChunk.Area.FirstColumn) * FieldCellSize - Pad, static_cast<float>(Field.GetRows() - InChunk.Area.FirstRow - InChunk.Area.Rows) * FieldCellSize - Pad);
	InParent->addChild(InOutSprite);
}

void TileMergeView::Upload(Chunk& InChunk, const BakeResult& InResult)
{
	const model::ProfileScope Scope("TileMerge.Upload");
	UploadLayer(InChunk, InResult.Region, InResult.Color, ChunkPadPixels, this, InChunk.Texture, InChunk.Image);
	if ( InResult.Shadow.IsEmpty() || !ShadowParent )
	{
		return;
	}
	UploadLayer(InChunk, InResult.Region, InResult.Shadow, 0, ShadowParent, InChunk.ShadowTexture, InChunk.ShadowImage);
}

void TileMergeView::BakeChunk(Chunk& InChunk)
{
	const CellRect Region = TakeDirty(InChunk);
	Upload(InChunk, BakeRegion(Field, InChunk, Region, ShadowParent && Settings.IsShadowEnabled()));
}

void TileMergeView::Flush()
{
	UploadGpu();
	DrainWorkers();
	for ( Chunk& Current : Chunks )
	{
		if ( !Current.Dirty.IsEmpty() )
		{
			BakeChunk(Current);
		}
	}
}

void TileMergeView::Submit(const std::size_t InIndex, const TSharedHandle<model::TileMergeField>& InSnapshot)
{
	Chunk& Current = Chunks[InIndex];
	const CellRect Region = TakeDirty(Current);
	const bool bShadow = ShadowParent && Settings.IsShadowEnabled();
	Current.bInFlight = true;
	++InFlightCount;
	model::WorkQueue& Worker = *Workers[NextWorker];
	NextWorker = (NextWorker + 1) % Workers.Size();
	Worker.Submit<BakeResult>(
		[this, InIndex, InSnapshot, Region, bShadow]() { return BakeRegion(InSnapshot->Get(), Chunks[InIndex], Region, bShadow); },
		[this, InIndex](BakeResult InResult)
		{
			Chunk& Done = Chunks[InIndex];
			Done.bInFlight = false;
			--InFlightCount;
			Upload(Done, InResult);
		});
}

int TileMergeView::Advance()
{
	if ( Gpu )
	{
		UploadGpu();
		return 0;
	}
	TSharedHandle<model::TileMergeField> Snapshot;
	for ( std::size_t Index = 0; Index < Chunks.Size(); ++Index )
	{
		const Chunk& Current = Chunks[Index];
		if ( Current.bInFlight || Current.Dirty.IsEmpty() )
		{
			continue;
		}
		if ( !Snapshot )
		{
			const model::ProfileScope Scope("TileMerge.SnapshotCopy");
			Snapshot = MakeSharedValue<model::TileMergeField>(Field);
		}
		Submit(Index, Snapshot);
	}
	const model::ProfileScope PumpScope("TileMerge.PumpOnMain");
	for ( TUniquePtr<model::WorkQueue>& Worker : Workers )
	{
		Worker->Pump(UploadBudget);
	}
	return CountPending();
}

void TileMergeView::DrainWorkers()
{
	for ( TUniquePtr<model::WorkQueue>& Worker : Workers )
	{
		Worker->WaitIdle();
		Worker->Pump();
	}
}

} // namespace game::view::cocos

#pragma once

#include "cocos2d.h"

#include "model/IMergeBaker.h"
#include "model/TileMergeBaker.h"
#include "model/RefCounted.h"
#include "model/TileMergeField.h"
#include "model/WorkQueue.h"
#include "templates/TContainers.h"
#include "templates/TUniquePtr.h"
#include "ui/TileMergeGpu.h"

namespace game::view::cocos
{
/**
 * Draws a TileMergeField as chunk sprites: the merged shape of the map's walls or floor modifiers, baked into RGBA
 * textures a few tiles wide. A changed cell re-bakes only the region around it (the part of a chunk its look can
 * reach), on worker threads from a snapshot of the field; the main thread only uploads the finished pixels, so
 * destroying a wall never stalls a frame, however many pixels a cell has. This is the Cocos side of the
 * merge; the field and the baker know no engine. Cell rows count from the top of the map, like the level rows.
 */
class TileMergeView : public cocos2d::Node
{
public:
	static TileMergeView* Create(const model::TileMergeSettings& InSettings, int InColumns, int InRows, unsigned InSeed, int InSubdivision);
	~TileMergeView() override;

	int GetSubdivision() const { return Subdivision; }
	// Field cells are a tile divided into Subdivision x Subdivision; rows count from the top.
	void SetCell(int InColumn, int InRow, model::MergeMaterial InMaterial, float InAmount);
	bool ToCell(const cocos2d::Vec2& InPosition, int& OutColumn, int& OutRow) const;
	void BakeAll();
	/** Bakes every dirty chunk now on this thread, for code that needs the finished picture at once. */
	void Flush();
	/** The name of the painting path in use: gpu, cached or reference. */
	const char* GetBackendName() const;
	/** Starts the dirty chunks on the workers and uploads what has finished; returns how many chunks are not done. */
	int Advance();
	/** Chunks waiting to be baked or being baked. */
	int CountPending() const;
	void ApplySettings(const model::TileMergeSettings& InSettings);
	/** Switches to other settings that may change the backend or the pixel density, keeping the cells; the picture is baked again. */
	void Reconfigure(const model::TileMergeSettings& InSettings);
	void SetShadowParent(cocos2d::Node* InParent);

	void update(float InDelta) override;

private:
	/** A rectangle of field cells, end exclusive. */
	struct CellRect
	{
		int FirstColumn = 0;
		int FirstRow = 0;
		int EndColumn = 0;
		int EndRow = 0;

		bool IsEmpty() const { return EndColumn <= FirstColumn || EndRow <= FirstRow; }
		void Add(const CellRect& InOther);
	};

	void ApplyFilter(cocos2d::Texture2D* InTexture) const;
	void RefreshFilter();

	bool bSmooth = true;

	struct Chunk
	{
		model::MergeChunk Area;
		cocos2d::Texture2D* Texture = nullptr;
		cocos2d::Sprite* Image = nullptr;
		cocos2d::Texture2D* ShadowTexture = nullptr;
		cocos2d::Sprite* ShadowImage = nullptr;
		/** Cells to re-bake; the whole chunk until its texture exists. */
		CellRect Dirty;
		bool bInFlight = false;
	};

	/** Pixels of one baked region, with where they go in the chunk's textures. */
	struct BakeResult
	{
		CellRect Region;
		TArray<std::uint8_t> Color;
		TArray<std::uint8_t> Shadow;
	};

	TileMergeView(const model::TileMergeSettings& InSettings, int InColumns, int InRows, unsigned InSeed, int InSubdivision);

	void BuildBackend();
	void ClearBackend();
	void MarkDirty(int InColumn, int InRow);
	void MarkWholeDirty(Chunk& InChunk);
	CellRect TakeDirty(Chunk& InChunk) const;
	BakeResult BakeRegion(const model::TileMergeField& InField, const Chunk& InChunk, const CellRect& InRegion, bool bInShadow) const;
	void Upload(Chunk& InChunk, const BakeResult& InResult);
	void UploadLayer(const Chunk& InChunk, const CellRect& InRegion, const TArray<std::uint8_t>& InPixels, int InPad, cocos2d::Node* InParent, cocos2d::Texture2D*& InOutTexture, cocos2d::Sprite*& InOutSprite);
	void BakeChunk(Chunk& InChunk);
	void Submit(std::size_t InIndex, const TSharedHandle<model::TileMergeField>& InSnapshot);
	void DrainWorkers();
	void UploadGpu();
	int ComputeReach() const;
	Chunk* FindChunk(int InColumn, int InRow);

	cocos2d::Node* ShadowParent = nullptr;
	model::TileMergeSettings Settings;
	model::TileMergeField Field;
	TUniquePtr<model::IMergeBaker> Baker;
	/** Set when the GPU path draws the shapes; then there are no chunks, baker or workers. */
	TUniquePtr<TileMergeGpu> Gpu;
	bool bGpuDirty = false;
	TArray<Chunk> Chunks;
	int Subdivision = 1;
	unsigned Seed = 0;
	int ChunkColumns = 0;
	int ChunkRows = 0;
	int ReachCells = 1;
	int InFlightCount = 0;
	std::size_t NextWorker = 0;
	/** Declared last so they stop first: a worker uses the baker and the settings. */
	TArray<TUniquePtr<model::WorkQueue>> Workers;
};
} // namespace game::view::cocos

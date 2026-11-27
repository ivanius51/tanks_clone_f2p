#pragma once

#include <cstddef>
#include <cstdint>

#include "model/IMergeBaker.h"
#include "model/TileMergeField.h"

namespace game::model
{
/** A rectangle of map cells to bake, with the pixels each cell gets. */
struct MergeChunk
{
	int FirstColumn = 0;
	int FirstRow = 0;
	int Columns = 0;
	int Rows = 0;
	int CellPixels = 64;
	int PadPixels = 0;

	int GetWidth() const { return Columns * CellPixels + 2 * PadPixels; }
	int GetHeight() const { return Rows * CellPixels + 2 * PadPixels; }
	size_t GetByteCount() const { return static_cast<size_t>(GetWidth()) * static_cast<size_t>(GetHeight()) * 4; }
};

/**
 * Paints the merged shape of a TileMergeField into RGBA pixels, chunk by chunk. Every material is generated per pixel
 * from a seeded noise (brick with mortar and plaster, stone with cracks and rust, wood planks, water, forest), so there
 * is no repeating pattern and no hand-made tile set; an overlay (snow, dirt, sand) specks the surface. Pixels depend
 * only on the map position and the seed, so chunks join without seams. With a subdivision the field cells are that
 * many times smaller than a tile (walls chip in sub-cells); textures still use whole-tile coordinates. Engine-free; row 0 of the output is the top.
 */
class TileMergeBaker : public IMergeBaker
{
public:
	TileMergeBaker(const TileMergeSettings& InSettings, std::uint32_t InSeed, int InSubdivision = 1);

	void SetSeed(std::uint32_t InSeed) override;
	void Bake(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const override;
	void BakeShadow(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const override;
	const char* GetName() const override { return "reference"; }

protected:
	/** The bake itself, for any sampler with the field's query methods (the field, or a cache of it). */
	template <typename TSampler>
	void BakeWith(const TSampler& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const;
	template <typename TSampler>
	void BakeShadowWith(const TSampler& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const;
	/** The same bake through a MergeFieldCache of the chunk's surroundings: identical pixels, much faster. */
	void BakeCached(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const;
	void BakeShadowCached(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const;
	/** Cells around a chunk that a pixel of it can read: the face, edge light and shadow reach plus the field's own search. */
	int ComputeReachCells() const;
	const TileMergeSettings& GetSettings() const { return Settings; }
	int GetSubdivision() const { return Subdivision; }

private:
	struct Rgb
	{
		float R = 0.0f;
		float G = 0.0f;
		float B = 0.0f;
	};

	float Hash(int InX, int InY) const;
	float Noise(float InX, float InY) const;
	Rgb Brick(float InU, float InV, float InX, float InY, float InEdge) const;
	Rgb Stone(float InU, float InV, float InX, float InY) const;
	Rgb Wood(float InU, float InV) const;
	Rgb Water(float InX, float InY) const;
	Rgb Forest(float InX, float InY) const;
	Rgb Dirt(float InX, float InY) const;
	Rgb Road(float InX, float InY, float InEdge) const;
	Rgb Fill(MergeMaterial InMaterial, float InX, float InY, float InEdge) const;
	template <typename TSampler>
	float EdgeLightShade(const TSampler& InField, float InX, float InY) const;
	template <typename TSampler>
	float FaceShade(const TSampler& InField, float InX, float InY) const;
	void AddOverlay(Rgb& InOut, float InX, float InY, int InPixelX, int InPixelY, float InEdge) const;

	const TileMergeSettings& Settings;
	std::uint32_t Seed;
	float CoverageThreshold;
	int Subdivision;
};
} // namespace game::model

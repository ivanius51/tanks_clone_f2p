#pragma once

#include <cstddef>
#include <cstdint>

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
class TileMergeBaker
{
public:
	TileMergeBaker(const TileMergeSettings& InSettings, std::uint32_t InSeed, int InSubdivision = 1);

	void SetSeed(std::uint32_t InSeed);
	void Bake(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const;
	void BakeShadow(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const;

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
	float EdgeLightShade(const TileMergeField& InField, float InX, float InY) const;
	float FaceShade(const TileMergeField& InField, float InX, float InY) const;
	void AddOverlay(Rgb& InOut, float InX, float InY, int InPixelX, int InPixelY, float InEdge) const;

	const TileMergeSettings& Settings;
	std::uint32_t Seed;
	float CoverageThreshold;
	int Subdivision;
};
} // namespace game::model

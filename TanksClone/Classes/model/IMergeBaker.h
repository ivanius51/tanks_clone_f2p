#pragma once

#include <cstdint>

#include "model/TileMergeField.h"

namespace game::model
{
struct MergeChunk;

/**
 * Paints a TileMergeField into RGBA pixels. Several implementations give the same picture at different speeds
 * (a plain reference one for old devices, a cached CPU one); the view picks one by name from the settings. Calls are
 * const and thread-safe, so workers may bake different chunks at once.
 */
class IMergeBaker
{
public:
	virtual ~IMergeBaker() = default;

	virtual void SetSeed(std::uint32_t InSeed) = 0;
	virtual void Bake(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const = 0;
	virtual void BakeShadow(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const = 0;
	virtual const char* GetName() const = 0;
};
} // namespace game::model

#pragma once

#include "model/TileMergeBaker.h"

namespace game::model
{
/**
 * The CPU baker with a cached copy of the field window around each bake (MergeFieldCache): same pixels as the
 * reference baker, several times faster, because the field queries that dominate a bake become array reads and
 * empty pixels skip the shading work altogether.
 */
class TileMergeBakerCached : public TileMergeBaker
{
public:
	using TileMergeBaker::TileMergeBaker;

	void Bake(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const override;
	void BakeShadow(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* OutRgba) const override;
	const char* GetName() const override { return "cached"; }
};
} // namespace game::model

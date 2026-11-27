#include "model/TileMergeBakerCached.h"

namespace game::model
{
void TileMergeBakerCached::Bake(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	BakeCached(InField, InChunk, OutRgba);
}

void TileMergeBakerCached::BakeShadow(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	BakeShadowCached(InField, InChunk, OutRgba);
}
} // namespace game::model

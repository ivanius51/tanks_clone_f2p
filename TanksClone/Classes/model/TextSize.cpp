#include "model/TextSize.h"

#include <algorithm>
#include <vector>

namespace game::model
{

static_assert(static_cast<int>(TextSize::Large) + 1 == static_cast<int>(TextScales::SizeCount), "one scale per text size, in order");

TextScales TextScales::FromConfig(const IConfigNode& InNode)
{
	TextScales Result;
	const std::vector<float> Read = InNode.GetValueOr("text_scales", std::vector<float>());
	for ( std::size_t Index = 0; Index < std::min(Read.size(), SizeCount); ++Index )
	{
		Result.Scales[Index] = std::max(MinScale, Read[Index]);
	}
	return Result;
}

float TextScales::Get(const TextSize InSize) const
{
	return Scales[static_cast<std::size_t>(InSize)];
}

TextSize TextScales::Next(const TextSize InSize)
{
	return FromIndex((static_cast<int>(InSize) + 1) % static_cast<int>(SizeCount));
}

TextSize TextScales::FromIndex(const int InIndex)
{
	return InIndex > 0 && InIndex < static_cast<int>(SizeCount) ? static_cast<TextSize>(InIndex) : TextSize::Small;
}

} // namespace game::model

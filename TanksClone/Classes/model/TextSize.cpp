#include "model/TextSize.h"
#include "templates/TContainers.h"

#include <algorithm>

namespace game::model
{

static_assert(static_cast<int>(ETextSize::Large) + 1 == static_cast<int>(TextScales::SizeCount), "one scale per text size, in order");

TextScales TextScales::FromConfig(const IConfigNode& InNode)
{
	TextScales Result;
	const TArray<float> Read = InNode.GetValueOr("text_scales", TArray<float>());
	for ( std::size_t Index = 0; Index < std::min(Read.Size(), SizeCount); ++Index )
	{
		Result.Scales[Index] = std::max(MinScale, Read[Index]);
	}
	return Result;
}

float TextScales::Get(const ETextSize InSize) const
{
	return Scales[static_cast<std::size_t>(InSize)];
}

ETextSize TextScales::Next(const ETextSize InSize)
{
	return FromIndex((static_cast<int>(InSize) + 1) % static_cast<int>(SizeCount));
}

ETextSize TextScales::FromIndex(const int InIndex)
{
	return InIndex > 0 && InIndex < static_cast<int>(SizeCount) ? static_cast<ETextSize>(InIndex) : ETextSize::Small;
}

} // namespace game::model

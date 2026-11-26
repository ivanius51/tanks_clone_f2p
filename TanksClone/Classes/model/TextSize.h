#pragma once

#include <array>

#include "model/ConfigNode.h"

namespace game::model
{
/** The text size the player picks in the settings. Stored in the profile by its number. */
enum class ETextSize
{
	Small = 0,
	Medium = 1,
	Large = 2,
	Count
};

/**
 * How much the text, the buttons and the spacing between them grow for each text size
 * ("text_scales" in configs/platform/display.json: one factor per size, smallest first).
 */
class TextScales
{
public:
	static constexpr SizeType SizeCount = 3;

	TextScales() = default;
	static TextScales FromConfig(const IConfigNode& InNode);

	float Get(ETextSize InSize) const;

	static ETextSize Next(ETextSize InSize);
	static ETextSize FromIndex(int InIndex);

	static constexpr float MinScale = 0.5f;

private:
	std::array<float, SizeCount> Scales = { 0.9f, 1.0f, 1.12f };
};
} // namespace game::model

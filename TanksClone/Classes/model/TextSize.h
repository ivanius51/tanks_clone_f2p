#pragma once

#include <array>

#include "model/ConfigNode.h"

namespace tanks::model
{

/** The text size the player picks in the settings. Stored in the profile by its number. */
enum class TextSize
{
	Small = 0,
	Medium = 1,
	Large = 2
};

/**
 * How much the text, the buttons and the spacing between them grow for each text size
 * ("text_scales" in configs/display.json: one factor per size, smallest first).
 */
class TextScales
{
public:
	static constexpr std::size_t SizeCount = 3;

	TextScales() = default;
	// Missing or bad factors keep their defaults; a factor is never below MinScale.
	static TextScales FromConfig(const IConfigNode& InNode);

	float Get(TextSize InSize) const;

	// Small -> Medium -> Large -> Small.
	static TextSize Next(TextSize InSize);
	// A stored number; anything unknown gives Small.
	static TextSize FromIndex(int InIndex);

	static constexpr float MinScale = 0.5f;

private:
	std::array<float, SizeCount> Scales = { 0.9f, 1.0f, 1.12f };
};

} // namespace tanks::model

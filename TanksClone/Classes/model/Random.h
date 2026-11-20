#pragma once

#include <cstdint>

namespace tanks::model
{

/**
 * Seeded pseudo-random numbers for all gameplay randomness. The same seed gives the same
 * sequence on every platform and compiler (own generator, no std distributions).
 */
class Random
{
public:
	explicit Random(std::uint64_t InSeed);

	void SetSeed(std::uint64_t InSeed);
	std::uint64_t GetSeed() const;

	// InMin..InMax inclusive; the bounds may come in any order.
	int Int(int InMin, int InMax);
	// InMin..InMax; the bounds may come in any order.
	float Float(float InMin, float InMax);
	// True with probability InChance (0..1).
	bool Chance(float InChance);

	std::uint32_t NextUInt32();

private:
	std::uint64_t Seed = 0;
	std::uint64_t State = 0;
};

// The generator gameplay code draws from until a game context is passed in explicitly.
// Seeded from the clock at start; autotests reseed it with a fixed value per level.
Random& GameRandom();

std::uint64_t MakeTimeSeed();

} // namespace tanks::model

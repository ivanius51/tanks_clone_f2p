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

	int Int(int InMin, int InMax);
	float Float(float InMin, float InMax);
	bool Chance(float InChance);

	std::uint32_t NextUInt32();

private:
	std::uint64_t Seed = 0;
	std::uint64_t State = 0;
};

Random& GameRandom();

std::uint64_t MakeTimeSeed();
} // namespace tanks::model

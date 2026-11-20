#include "model/Random.h"

#include <chrono>
#include <utility>

namespace tanks::model
{

Random::Random(const std::uint64_t InSeed)
{
	SetSeed(InSeed);
}

void Random::SetSeed(const std::uint64_t InSeed)
{
	Seed = InSeed;
	State = InSeed;
}

std::uint64_t Random::GetSeed() const
{
	return Seed;
}

// SplitMix64: tiny, fast and identical everywhere.
std::uint32_t Random::NextUInt32()
{
	State += 0x9E3779B97F4A7C15ull;
	std::uint64_t Mixed = State;
	Mixed = (Mixed ^ (Mixed >> 30)) * 0xBF58476D1CE4E5B9ull;
	Mixed = (Mixed ^ (Mixed >> 27)) * 0x94D049BB133111EBull;
	Mixed ^= Mixed >> 31;
	return static_cast<std::uint32_t>(Mixed >> 32);
}

int Random::Int(int InMin, int InMax)
{
	if ( InMin > InMax )
	{
		std::swap(InMin, InMax);
	}
	const std::uint64_t Span = static_cast<std::uint64_t>(static_cast<std::int64_t>(InMax) - InMin) + 1;
	return static_cast<int>(InMin + static_cast<std::int64_t>((static_cast<std::uint64_t>(NextUInt32()) * Span) >> 32));
}

float Random::Float(float InMin, float InMax)
{
	if ( InMin > InMax )
	{
		std::swap(InMin, InMax);
	}
	const float Unit = static_cast<float>(NextUInt32() >> 8) / static_cast<float>(1u << 24);
	return InMin + (InMax - InMin) * Unit;
}

bool Random::Chance(const float InChance)
{
	return Float(0.0f, 1.0f) < InChance;
}

std::uint64_t MakeTimeSeed()
{
	return static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
}

Random& GameRandom()
{
	static Random Instance(MakeTimeSeed());
	return Instance;
}

} // namespace tanks::model

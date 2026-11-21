#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

namespace tanks
{

// Numbers that describe a tank. To add a stat: add a value here (before Count),
// a name in Stats.cpp and use it where it matters.
enum class StatType
{
	MaxHealth = 0,
	Damage,
	Armor,
	Speed,
	ReloadTime, // seconds between shots: a multiplier below 1 reloads faster
	MaxAmmo,
	TurnSpeed, // multiplier of the turn rate: above 1 turns faster
	Count
};

// False (OutStat untouched) for an unknown name. Lets IConfigNode read a StatType by name.
bool EnumFromName(std::string_view InName, StatType& OutStat);

// Changes a stat: final = (base + sum of all add) * product of all mul.
struct StatModifier
{
	StatType Stat = StatType::Damage;
	float Add = 0.0f;
	float Mul = 1.0f;
};

// Base values of the stats plus the current modifiers.
class StatSet
{
public:
	StatSet();

	void SetBase(StatType InStat, float InValue);
	float GetBase(StatType InStat) const;
	// The value with all modifiers applied.
	float Get(StatType InStat) const;
	int GetInt(StatType InStat) const;

	// Replaces the current modifiers.
	void ApplyModifiers(const std::vector<StatModifier>& InModifiers);

private:
	static constexpr std::size_t Count = static_cast<size_t>(StatType::Count);

	std::array<float, Count> Base;
	std::array<float, Count> Final;
};

} // namespace tanks

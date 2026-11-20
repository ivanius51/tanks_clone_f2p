#pragma once

#include <array>
#include <vector>
#include "cocos2d.h"

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
	Count
};

StatType ParseStatType(const cocos2d::Value& InValue, bool* InOk = nullptr);

// Changes a stat: final = (base + sum of all add) * product of all mul.
struct StatModifier
{
	StatType Stat = StatType::Damage;
	float Add = 0.0f;
	float Mul = 1.0f;
};

// Reads {"stat": "Speed", "add": 10, "mul": 1.5} entries. Unknown entries are skipped.
std::vector<StatModifier> ParseStatModifiers(const cocos2d::Value& InValue);

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
	static constexpr size_t Count = static_cast<size_t>(StatType::Count);

	std::array<float, Count> Base;
	std::array<float, Count> Final;
};

} // namespace tanks

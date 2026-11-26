#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include "templates/TContainers.h"

namespace game
{
enum class StatType
{
	MaxHealth = 0,
	Damage,
	Armor,
	Speed,
	ReloadTime,
	MaxAmmo,
	TurnSpeed,
	Count
};

bool EnumFromName(std::string_view InName, StatType& OutStat);

/** Changes a stat: final = (base + sum of all add) * product of all mul. */
struct StatModifier
{
	StatType Stat = StatType::Damage;
	float Add = 0.0f;
	float Mul = 1.0f;
};

/** Base values of the stats plus the current modifiers. */
class StatSet
{
public:
	StatSet();

	void SetBase(StatType InStat, float InValue);
	float GetBase(StatType InStat) const;
	float Get(StatType InStat) const;
	int GetInt(StatType InStat) const;

	void ApplyModifiers(const TArray<StatModifier>& InModifiers);

private:
	static constexpr std::size_t Count = static_cast<size_t>(StatType::Count);

	std::array<float, Count> Base;
	std::array<float, Count> Final;
};
} // namespace game

#include "game/components/Stats.h"

#include <array>
#include <cmath>

namespace tanks
{

namespace
{
	struct StatName
	{
		StatType Stat;
		const char* Name;
	};

	const std::array<StatName, 6> StatNames = { {
		{ StatType::MaxHealth, "MaxHealth" },
		{ StatType::Damage, "Damage" },
		{ StatType::Armor, "Armor" },
		{ StatType::Speed, "Speed" },
		{ StatType::ReloadTime, "ReloadTime" },
		{ StatType::MaxAmmo, "MaxAmmo" }
	} };

	size_t IndexOf(const StatType InStat)
	{
		return static_cast<size_t>(InStat);
	}
}

bool EnumFromName(const std::string_view InName, StatType& OutStat)
{
	for ( const auto& CurrentEntry : StatNames )
	{
		if ( InName == CurrentEntry.Name )
		{
			OutStat = CurrentEntry.Stat;
			return true;
		}
	}
	return false;
}

StatSet::StatSet()
{
	Base.fill(0.0f);
	Final.fill(0.0f);
}

void StatSet::SetBase(const StatType InStat, const float InValue)
{
	Base[IndexOf(InStat)] = InValue;
	Final[IndexOf(InStat)] = InValue;
}

float StatSet::GetBase(const StatType InStat) const
{
	return Base[IndexOf(InStat)];
}

float StatSet::Get(const StatType InStat) const
{
	return Final[IndexOf(InStat)];
}

int StatSet::GetInt(const StatType InStat) const
{
	return static_cast<int>(std::lround(Get(InStat)));
}

void StatSet::ApplyModifiers(const std::vector<StatModifier>& InModifiers)
{
	std::array<float, Count> CurrentAdd;
	std::array<float, Count> CurrentMul;
	CurrentAdd.fill(0.0f);
	CurrentMul.fill(1.0f);

	for ( const auto& Modifier : InModifiers )
	{
		CurrentAdd[IndexOf(Modifier.Stat)] += Modifier.Add;
		CurrentMul[IndexOf(Modifier.Stat)] *= Modifier.Mul;
	}
	for ( size_t Index = 0; Index < Count; ++Index )
	{
		Final[Index] = (Base[Index] + CurrentAdd[Index]) * CurrentMul[Index];
	}
}

} // namespace tanks

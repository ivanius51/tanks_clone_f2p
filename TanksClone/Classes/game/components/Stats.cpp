#include "game/components/Stats.h"

#include <cmath>
#include <array>
#include "core/DataConverter.h"

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

StatType ParseStatType(const cocos2d::Value& InValue, bool* const InOk)
{
	if ( InOk )
	{
		*InOk = false;
	}
	if ( InValue.getType() == cocos2d::Value::Type::STRING )
	{
		for ( const auto& CurrentEntry : StatNames )
		{
			if ( InValue.asString() == CurrentEntry.Name )
			{
				if ( InOk )
				{
					*InOk = true;
				}
				return CurrentEntry.Stat;
			}
		}
	}
	return StatType::Damage;
}

std::vector<StatModifier> ParseStatModifiers(const cocos2d::Value& InValue)
{
	std::vector<StatModifier> Result;
	if ( InValue.getType() != cocos2d::Value::Type::VECTOR )
	{
		return Result;
	}

	for ( const auto& Item : InValue.asValueVector() )
	{
		if ( Item.getType() != cocos2d::Value::Type::MAP )
		{
			continue;
		}
		const cocos2d::ValueMap& Map = Item.asValueMap();

		auto StatIt = Map.find("stat");
		bool bOk = false;
		StatModifier Modifier;
		if ( StatIt != Map.end() )
		{
			Modifier.Stat = ParseStatType(StatIt->second, &bOk);
		}
		if ( !bOk )
		{
			CCLOGERROR("Stats: unknown stat in a modifier");
			continue;
		}
		Modifier.Add = DataConverter::GetFloat(Map, "add", 0.0f);
		Modifier.Mul = DataConverter::GetFloat(Map, "mul", 1.0f);
		Result.push_back(Modifier);
	}
	return Result;
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

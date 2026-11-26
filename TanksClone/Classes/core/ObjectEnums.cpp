#include "core/ObjectEnums.h"
#include "templates/TContainers.h"

#include <array>
#include <initializer_list>
#include <string>
#include <utility>

namespace game
{

namespace
{

template <typename E>
E ParseEnum(const model::JsonValue& InValue, std::initializer_list<std::pair<const char*, E>> InNames, E InFallback)
{
	if ( InValue.IsString() )
	{
		const std::string CurrentName = InValue.AsString();
		for ( const auto& CurrentEntry : InNames )
		{
			if ( CurrentName == CurrentEntry.first )
			{
				return CurrentEntry.second;
			}
		}
		return InFallback;
	}
	return InValue.GetType() == model::JsonValue::Type::Int ? static_cast<E>(InValue.AsInt()) : InFallback;
}

} // namespace

EObjectType ParseObjectType(const model::JsonValue& InValue)
{
	return ParseEnum(InValue, {
		{ "Vehicle", EObjectType::Vehicle },
		{ "Bullet", EObjectType::Bullet },
		{ "Wall", EObjectType::Wall },
		{ "Ground", EObjectType::Ground },
		{ "Bonus", EObjectType::Bonus }
	}, EObjectType::Undefined);
}

ENodeType ParseNodeType(const model::JsonValue& InValue)
{
	return ParseEnum(InValue, {
		{ "Node", ENodeType::Node },
		{ "Sprite", ENodeType::Sprite },
		{ "ImageView", ENodeType::ImageView },
		{ "Button", ENodeType::Button },
		{ "Label", ENodeType::Label }
	}, ENodeType::Node);
}

EVehicleType ParseVehicleType(const model::JsonValue& InValue)
{
	return ParseEnum(InValue, {
		{ "Tank", EVehicleType::Tank }
	}, EVehicleType::Undefined);
}

ETeam ParseTeam(const model::JsonValue& InValue)
{
	return ParseEnum(InValue, {
		{ "Player", ETeam::Player },
		{ "Enemy", ETeam::Enemy },
		{ "Neutral", ETeam::Neutral }
	}, ETeam::Neutral);
}

EBonusType ParseBonusType(const model::JsonValue& InValue)
{
	return ParseEnum(InValue, {
		{ "Ammo", EBonusType::Ammo },
		{ "Repair", EBonusType::Repair },
		{ "Buff", EBonusType::Buff },
		{ "Coins", EBonusType::Coins }
	}, EBonusType::Undefined);
}

bool EnumFromName(const std::string_view InName, EAiType& OutType)
{
	constexpr std::array<std::pair<std::string_view, EAiType>, 2> AiNames = { {
		{ "Wander", EAiType::Wander },
		{ "Hunter", EAiType::Hunter }
	} };
	for ( const auto& [Name, Type] : AiNames )
	{
		if ( InName == Name )
		{
			OutType = Type;
			return true;
		}
	}
	return false;
}

namespace
{
	struct EnemyName
	{
		EEnemyType Kind;
		const char* Name;
	};

	// The one place that lists the enemy classes.
	const std::array<EnemyName, 4> EnemyNames = { {
		{ EEnemyType::Light, "Light" },
		{ EEnemyType::Medium, "Medium" },
		{ EEnemyType::Heavy, "Heavy" },
		{ EEnemyType::Boss, "Boss" }
	} };
}

bool EnumFromName(const std::string_view InName, EEnemyType& OutType)
{
	for ( const auto& CurrentEntry : EnemyNames )
	{
		if ( InName == CurrentEntry.Name )
		{
			OutType = CurrentEntry.Kind;
			return true;
		}
	}
	return false;
}

const char* EnemyTypeName(const EEnemyType InType)
{
	for ( const auto& CurrentEntry : EnemyNames )
	{
		if ( CurrentEntry.Kind == InType )
		{
			return CurrentEntry.Name;
		}
	}
	return "Medium";
}

const TArray<EEnemyType>& AllEnemyTypes()
{
	static const TArray<EEnemyType> TYPES = []()
	{
		TArray<EEnemyType> CurrentTypes;
		for ( const auto& CurrentEntry : EnemyNames )
		{
			CurrentTypes.PushBack(CurrentEntry.Kind);
		}
		return CurrentTypes;
	}();
	return TYPES;
}

} // namespace game

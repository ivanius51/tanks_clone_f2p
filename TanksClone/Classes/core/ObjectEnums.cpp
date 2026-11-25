#include "core/ObjectEnums.h"

#include <array>
#include <initializer_list>
#include <string>
#include <utility>

namespace game
{

namespace
{

template <typename E>
E ParseEnum(const cocos2d::Value& InValue, std::initializer_list<std::pair<const char*, E>> InNames, E InFallback)
{
	switch ( InValue.getType() )
	{
		case cocos2d::Value::Type::STRING:
		{
			const std::string CurrentName = InValue.asString();
			for ( const auto& CurrentEntry : InNames )
			{
				if ( CurrentName == CurrentEntry.first )
				{
					return CurrentEntry.second;
				}
			}
			return InFallback;
		}
		case cocos2d::Value::Type::INTEGER:
			return static_cast<E>(InValue.asInt());
		default:
			return InFallback;
	}
}

} // namespace

ObjectType ParseObjectType(const cocos2d::Value& InValue)
{
	return ParseEnum(InValue, {
		{ "Vehicle", ObjectType::Vehicle },
		{ "Bullet", ObjectType::Bullet },
		{ "Wall", ObjectType::Wall },
		{ "Ground", ObjectType::Ground },
		{ "Bonus", ObjectType::Bonus }
	}, ObjectType::Undefined);
}

NodeType ParseNodeType(const cocos2d::Value& InValue)
{
	return ParseEnum(InValue, {
		{ "Node", NodeType::Node },
		{ "Sprite", NodeType::Sprite },
		{ "ImageView", NodeType::ImageView },
		{ "Button", NodeType::Button },
		{ "Label", NodeType::Label }
	}, NodeType::Node);
}

VehicleType ParseVehicleType(const cocos2d::Value& InValue)
{
	return ParseEnum(InValue, {
		{ "Tank", VehicleType::Tank }
	}, VehicleType::Undefined);
}

Team ParseTeam(const cocos2d::Value& InValue)
{
	return ParseEnum(InValue, {
		{ "Player", Team::Player },
		{ "Enemy", Team::Enemy },
		{ "Neutral", Team::Neutral }
	}, Team::Neutral);
}

BonusType ParseBonusType(const cocos2d::Value& InValue)
{
	return ParseEnum(InValue, {
		{ "Ammo", BonusType::Ammo },
		{ "Repair", BonusType::Repair },
		{ "Buff", BonusType::Buff },
		{ "Coins", BonusType::Coins }
	}, BonusType::Undefined);
}

bool EnumFromName(const std::string_view InName, AiType& OutType)
{
	constexpr std::array<std::pair<std::string_view, AiType>, 2> AiNames = { {
		{ "Wander", AiType::Wander },
		{ "Hunter", AiType::Hunter }
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
		EnemyType Kind;
		const char* Name;
	};

	// The one place that lists the enemy classes.
	const std::array<EnemyName, 4> EnemyNames = { {
		{ EnemyType::Light, "Light" },
		{ EnemyType::Medium, "Medium" },
		{ EnemyType::Heavy, "Heavy" },
		{ EnemyType::Boss, "Boss" }
	} };
}

bool EnumFromName(const std::string_view InName, EnemyType& OutType)
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

const char* EnemyTypeName(const EnemyType InType)
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

const std::vector<EnemyType>& AllEnemyTypes()
{
	static const std::vector<EnemyType> TYPES = []()
	{
		std::vector<EnemyType> CurrentTypes;
		for ( const auto& CurrentEntry : EnemyNames )
		{
			CurrentTypes.push_back(CurrentEntry.Kind);
		}
		return CurrentTypes;
	}();
	return TYPES;
}

} // namespace game

#pragma once

#include <string_view>
#include <vector>

#include "cocos2d.h"

namespace tanks
{
enum class ObjectType
{
	Undefined = 0,
	Vehicle,
	Bullet,
	Wall,
	Ground,
	Bonus
};

enum class NodeType
{
	Node = 0,
	Sprite,
	ImageView,
	Button,
	Label
};

enum class CollisionType
{
	Rect = 0,
	Circle
};

enum class CommandType
{
	Undefined = 0,
	MoveForward,
	MoveBack,
	MoveLeft,
	MoveRight,
	Stop,
	Shoot,
	StartFiring,
	StopFiring,
	UseAbility,
	SwitchWeapon
};

enum class ActionTag
{
	Move,
	Rotate,
	Stop,
	Animation,
	WaitMove,
	WaitRotate,
	WaitStop
};

enum class VehicleType
{
	Undefined = 0,
	Tank
};

enum class Team
{
	Neutral = 0,
	Player,
	Enemy
};

enum class BonusType
{
	Undefined = 0,
	Ammo,
	Repair,
	Buff,
	Coins
};

enum class EnemyType
{
	Light = 0,
	Medium,
	Heavy,
	Boss
};

enum class AiType
{
	None = 0,
	Wander,
	Hunter
};

ObjectType ParseObjectType(const cocos2d::Value& InValue);
NodeType ParseNodeType(const cocos2d::Value& InValue);
VehicleType ParseVehicleType(const cocos2d::Value& InValue);
Team ParseTeam(const cocos2d::Value& InValue);
BonusType ParseBonusType(const cocos2d::Value& InValue);
bool EnumFromName(std::string_view InName, AiType& OutType);
bool EnumFromName(std::string_view InName, EnemyType& OutType);

const char* EnemyTypeName(EnemyType InType);
const std::vector<EnemyType>& AllEnemyTypes();

constexpr int ToTag(const ActionTag InTag)
{
	return static_cast<int>(InTag);
}
} // namespace tanks

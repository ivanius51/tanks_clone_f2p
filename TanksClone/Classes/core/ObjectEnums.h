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
	StartFiring, // the fire key went down: keep firing while it is held
	StopFiring,
	UseAbility, // the player's second skill (shield etc.)
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

// Who an object fights for. Bullets never hurt their own team.
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
	Buff // gives a buff (named in the bonus config)
};

// The class of an enemy tank (not a particular enemy!). Particular enemies are JSON files in
// configs/enemies/ that combine stats, abilities and buffs and say which class they belong to.
// The class is used to group enemies (editor, difficulty) and can be extended freely.
enum class EnemyType
{
	Light = 0,
	Medium,
	Heavy,
	Boss
};

// How an AI-controlled tank behaves.
enum class AiType
{
	None = 0,
	Wander, // roams at random, shoots when it happens to face the player
	Hunter  // moves toward the player and shoots when lined up
};

// Config values may name an enumerator ("Vehicle") or hold its integer value.
// Unknown values give the first enumerator (Undefined / None / Neutral / Node).
ObjectType ParseObjectType(const cocos2d::Value& InValue);
NodeType ParseNodeType(const cocos2d::Value& InValue);
VehicleType ParseVehicleType(const cocos2d::Value& InValue);
Team ParseTeam(const cocos2d::Value& InValue);
BonusType ParseBonusType(const cocos2d::Value& InValue);
// False (OutType untouched) for an unknown name. Lets IConfigNode read an AiType by name.
bool EnumFromName(std::string_view InName, AiType& OutType);
// False (OutType untouched) for an unknown class name. Lets IConfigNode read an EnemyType by name.
bool EnumFromName(std::string_view InName, EnemyType& OutType);

// Name of the class in enemy configs ("enemy_type": "Heavy").
const char* EnemyTypeName(EnemyType InType);
const std::vector<EnemyType>& AllEnemyTypes();

// Tag value to pass to cocos2d::Action::setTag / Node::getActionByTag.
constexpr int ToTag(const ActionTag InTag)
{
	return static_cast<int>(InTag);
}

} // namespace tanks

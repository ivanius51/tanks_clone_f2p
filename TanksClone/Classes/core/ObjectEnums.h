#pragma once

#include <string_view>

#include "model/JsonValue.h"
#include "templates/TContainers.h"

namespace game
{
enum class EObjectType
{
	Undefined = 0,
	Vehicle,
	Bullet,
	Wall,
	Ground,
	Bonus,
	Count
};

enum class ENodeType
{
	Node = 0,
	Sprite,
	ImageView,
	Button,
	Label,
	Count
};

enum class ECollisionType
{
	Rect = 0,
	Circle,
	Count
};

enum class ECommandType
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
	SwitchWeapon,
	Count
};

enum class EActionTag
{
	Move,
	Rotate,
	Stop,
	Animation,
	WaitMove,
	WaitRotate,
	WaitStop,
	Count
};

enum class EVehicleType
{
	Undefined = 0,
	Tank,
	Count
};

enum class ETeam
{
	Neutral = 0,
	Player,
	Enemy,
	Count
};

enum class EBonusType
{
	Undefined = 0,
	Ammo,
	Repair,
	Buff,
	Coins,
	Count
};

enum class EEnemyType
{
	Light = 0,
	Medium,
	Heavy,
	Boss,
	Count
};

enum class EAiType
{
	None = 0,
	Wander,
	Hunter,
	Count
};

EObjectType ParseObjectType(const model::JsonValue& InValue);
ENodeType ParseNodeType(const model::JsonValue& InValue);
EVehicleType ParseVehicleType(const model::JsonValue& InValue);
ETeam ParseTeam(const model::JsonValue& InValue);
EBonusType ParseBonusType(const model::JsonValue& InValue);
bool EnumFromName(std::string_view InName, EAiType& OutType);
bool EnumFromName(std::string_view InName, EEnemyType& OutType);

const char* EnemyTypeName(EEnemyType InType);
const TArray<EEnemyType>& AllEnemyTypes();

constexpr int ToTag(const EActionTag InTag)
{
	return static_cast<int>(InTag);
}
} // namespace game

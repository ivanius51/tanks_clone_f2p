#ifndef Object_Types
#define Object_Types
enum class eObjectType
{
	OBJECT_TYPE_UNDEFINED = 0,
	OBJECT_TYPE_VEHICLE,
	OBJECT_TYPE_BULLET,
	OBJECT_TYPE_WALL,
	OBJECT_TYPE_GROUND,
	OBJECT_TYPE_BONUS
};

enum class eNodeType
{
	NODE_TYPE_NODE = 0,
	NODE_TYPE_SPRITE,
	NODE_TYPE_IMAGE_VIEW,
	NODE_TYPE_BUTTON,
	NODE_TYPE_LABEL
};

enum class eCollisionType
{
	COLLISION_TYPE_RECT = 0,
	COLLISION_TYPE_CIRCLE
};

enum class eCommandType
{
	COMMAND_TYPE_UNDEFINED = 0,
	COMMAND_TYPE_MOVE_FORWARD,
	COMMAND_TYPE_MOVE_BACK,
	COMMAND_TYPE_MOVE_LEFT,
	COMMAND_TYPE_MOVE_RIGHT,
	COMMAND_TYPE_STOP,
	COMMAND_TYPE_SHOOT
};

enum class eActionTag
{
	ACTION_MOVE
};

enum class eWallType
{

};

enum class eVehicleType
{
	VEHICLE_TYPE_TANK
};

enum class eGroundType
{

};

enum class eBonusType
{

};

enum class eBulletType
{

};
#endif//Object_Types


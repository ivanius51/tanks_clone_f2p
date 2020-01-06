#ifndef Object_Types
#define Object_Types
enum class eObjectType
{
	OBJECT_TYPE_UNDEFINED = 0,
	OBJECT_TYPE_VEHICLE,
	OBJECT_TYPE_BULLET,
	OBJECT_TYPE_WALL,
	OBJECT_TYPE_GROUND,
	OBJECT_TYPE_BONUS,
	OBJECT_TYPE_END
};

enum class eCollisionType
{
	COLLISION_TYPE_RECT = 0,
	COLLISION_TYPE_CIRCLE
};

enum class eCommandType
{
	COMMAND_TYPE_UNDEFINED = 0,
	COMMAND_TYPE_MOVE,
	COMMAND_TYPE_RUN,
	COMMAND_TYPE_STOP,
	COMMAND_TYPE_ROTATE
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


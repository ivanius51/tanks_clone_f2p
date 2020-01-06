#include "Tank.h"

NS_CC_BEGIN


Tank::Tank()
{
}

Tank::~Tank()
{
}

Tank* Tank::create(Node* aNode)
{
	Tank* ret = new (std::nothrow) Tank();
	if ( ret && ret->build() )
	{
		ret->setVisualNode(aNode);
	}
	else
	{
		CC_SAFE_DELETE(ret);
	}
	return ret;
}

bool Tank::build()
{
	bool result = IMovableObject::build();
	setType(eObjectType::OBJECT_TYPE_VEHICLE);
	return result;
}

void Tank::onCommandReceived(GameCommand aCommand)
{
	switch ( aCommand.getType() )
	{
		case eCommandType::COMMAND_TYPE_UNDEFINED:
			break;
		case eCommandType::COMMAND_TYPE_MOVE_FORWARD:
			move();
			break;
		case eCommandType::COMMAND_TYPE_MOVE_BACK:
			break;
		case eCommandType::COMMAND_TYPE_MOVE_LEFT:
			break;
		case eCommandType::COMMAND_TYPE_MOVE_RIGHT:
			break;
		case eCommandType::COMMAND_TYPE_STOP:
			stop();
			break;
		case eCommandType::COMMAND_TYPE_SHOOT:
			break;
		default:
			break;
	}
}

NS_CC_END

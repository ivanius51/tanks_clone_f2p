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
	float delayTime = 0.0f;

#define UP_DEGREES 0
#define DOWN_DEGREES 180
#define LEFT_DEGREES 270
#define RIGHT_DEGREES 90

	switch ( aCommand.getType() )
	{
		case eCommandType::COMMAND_TYPE_UNDEFINED:
			break;
		case eCommandType::COMMAND_TYPE_MOVE_FORWARD:
			delayTime = turnTo(UP_DEGREES);
			break;
		case eCommandType::COMMAND_TYPE_MOVE_BACK:
			delayTime = turnTo(DOWN_DEGREES);
			break;
		case eCommandType::COMMAND_TYPE_MOVE_LEFT:
			delayTime = turnTo(LEFT_DEGREES);
			break;
		case eCommandType::COMMAND_TYPE_MOVE_RIGHT:
			delayTime = turnTo(RIGHT_DEGREES);
			break;
		case eCommandType::COMMAND_TYPE_STOP:
			stop();
			break;
		case eCommandType::COMMAND_TYPE_SHOOT:
			break;
		default:
			break;
	}

	switch ( aCommand.getType() )
	{
		case eCommandType::COMMAND_TYPE_UNDEFINED:
			break;
		case eCommandType::COMMAND_TYPE_MOVE_FORWARD:
		case eCommandType::COMMAND_TYPE_MOVE_BACK:
		case eCommandType::COMMAND_TYPE_MOVE_LEFT:
		case eCommandType::COMMAND_TYPE_MOVE_RIGHT:
			if ( delayTime > FLT_EPSILON )
			{
				getVisualNode()->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_WAIT_MOVE));
				auto waitForMoveAction = getVisualNode()->runAction(
					Sequence::createWithTwoActions(
						DelayTime::create(delayTime),
						CallFunc::create(CC_CALLBACK_0(IMovableObject::move, this))
					));
				waitForMoveAction->setTag(static_cast<int>(eActionTag::ACTION_TAG_WAIT_MOVE));
			}
			else
			{
				move();
			}
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

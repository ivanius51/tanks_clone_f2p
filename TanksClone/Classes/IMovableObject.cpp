#include "IMovableObject.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionEase.h"

NS_CC_BEGIN

#define UP_DEGREES (0)
#define DOWN_DEGREES (180)
#define LEFT_DEGREES (270)
#define RIGHT_DEGREES (90)
#define M_PI_DIV_180 (M_PI / 180.0f)

IMovableObject::IMovableObject()
	: mIsRunning(false)
	, mIsRotating(false)
	, mSpeed(50.0f)
	, mDirection(0.0f, 1.0f)
{
	
}

void IMovableObject::onMoveStart()
{
	mIsRunning = true;
}

void IMovableObject::onMoveEnd()
{
	mIsRunning = false;
}

void IMovableObject::updateDirrection(float aAngle)
{
	float radians = aAngle * M_PI_DIV_180;
	//0 90 180 270 360
	//0 1   0		-1   0
	//1 0		-1	 0	 1
	mDirection.x = sinf(radians);
	mDirection.y = cosf(radians);
}

IMovableObject::~IMovableObject()
{
}

bool IMovableObject::build()
{
	bool result = IPhysicsObject::build();
	setMovable(true);
	Director::getInstance()->getScheduler()->schedule(
		CC_CALLBACK_1(IMovableObject::update, this), this,
		(1.0f / 60.0f), CC_REPEAT_FOREVER, 0.0f, false, "UPDATE"
	);
	return result;
}

void IMovableObject::update(float aDelta)
{
	auto node = getVisualNode();
	if ( node )
	{
		if ( mIsRunning 
			&& !mIsRotating )
		{
			float distance = aDelta * mSpeed;
			Vec2 nextPosition = node->getPosition();
			nextPosition.x += distance * mDirection.x;
			nextPosition.y += distance * mDirection.y;
			node->setPosition(nextPosition);
		}
	}
}

void IMovableObject::move()
{
	auto node = getVisualNode();
	if ( node )
	{
		if ( !mIsRunning )
		{
			onMoveStart();
		}
		auto animationMove = dynamic_cast<Speed*>(node->getActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ANIMATION)));
		if ( animationMove )
		{
			animationMove->setSpeed(1.0f);
		}
		/*
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_MOVE));
		auto moveAction = node->runAction(
			RepeatForever::create(
					MoveBy::create(1.0f * 0.05f, Vec2(mSpeed * mDirection.x, mSpeed * mDirection.y) * 0.05f)
			)
		);
		moveAction->setTag(static_cast<int>(eActionTag::ACTION_TAG_MOVE));
		*/
	}
}
void IMovableObject::run()
{
	mIsRunning = true;
}
void IMovableObject::stop()
{
	if ( mIsRunning )
	{
		onMoveEnd();
	}
	mIsRotating = false;
	mIsRunning = false;
	auto node = getVisualNode();
	if ( node )
	{
		auto animationMove = dynamic_cast<Speed*>(node->getActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ANIMATION)));
		if ( animationMove )
		{
			animationMove->setSpeed(0.0f);
		}
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_WAIT_MOVE));
		//node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_MOVE));
	}
}
float IMovableObject::turnOn(float aAngle)
{
	float result = 0.0f;
	auto node = getVisualNode();
	if ( node )
	{
		auto oldDirection = mDirection;
		updateDirrection((node->getRotation() + aAngle));
		if ( oldDirection != mDirection )
		{
			stop();
			auto animationMove = dynamic_cast<Speed*>(node->getActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ANIMATION)));
			if ( animationMove )
			{
				animationMove->setSpeed(1.5f);
			}
			mIsRotating = true;
			node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
			auto action = node->runAction(Sequence::createWithTwoActions(
				EaseOut::create(RotateBy::create(mSpeed * 0.005, aAngle), 2.0f),
				CallFunc::create([=](){mIsRotating=false;})
			));
			action->setTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
			result = static_cast<ActionInterval*>(action)->getDuration();
		}
	}
	return result;
}
float IMovableObject::turnTo(float aAngle)
{
	float result = 0.0f;
	auto node = getVisualNode();
	if ( node )
	{
		auto oldDirection = mDirection;
		updateDirrection(aAngle);
		if ( oldDirection != mDirection )
		{
			stop();
			auto animationMove = dynamic_cast<Speed*>(node->getActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ANIMATION)));
			if ( animationMove )
			{
				animationMove->setSpeed(1.5f);
			}
			mIsRotating = true;
			node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
			auto action = node->runAction(Sequence::createWithTwoActions(
				EaseOut::create(RotateTo::create(mSpeed * 0.005, aAngle), 2.0f),
				CallFunc::create([=](){mIsRotating=false;})
			));
			action->setTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
			result = static_cast<ActionInterval*>(action)->getDuration();
		}
	}
	return result;
}

void IMovableObject::onCommandReceived(GameCommand aCommand)
{
	float delayTime = 0.0f;

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
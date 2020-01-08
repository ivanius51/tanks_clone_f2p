#include "IMovableObject.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionEase.h"

NS_CC_BEGIN
IMovableObject::IMovableObject()
	: mIsRunning(false)
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

IMovableObject::~IMovableObject()
{
}

bool IMovableObject::build()
{
	bool result = IPhysicsObject::build();
	setMovable(true);
	return result;
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
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_MOVE));
		auto moveAction = node->runAction(
			RepeatForever::create(
					MoveBy::create(1.0f * 0.05f, Vec2(mSpeed * mDirection.x, mSpeed * mDirection.y) * 0.05f)
			)
		);
		moveAction->setTag(static_cast<int>(eActionTag::ACTION_TAG_MOVE));
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
		stop();

		float degrees = ((node->getRotation() + aAngle) * M_PI) / 180.0f;
															   //0 90 180 270 360
		mDirection.x = sinf(degrees);//0 1   0	-1   0
		mDirection.y = cosf(degrees);//1 0	-1	 0	 1
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
		auto action = node->runAction(EaseOut::create(RotateBy::create(mSpeed * 0.005, aAngle), 2.0f));
		action->setTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
		result = static_cast<ActionInterval*>(action)->getDuration();
	}
	return result;
}
float IMovableObject::turnTo(float aAngle)
{
	float result = 0.0f;
	auto node = getVisualNode();
	if ( node
		&& fabs(aAngle - node->getRotation())>FLT_EPSILON)
	{
		stop();
		float degrees = (aAngle * M_PI) / 180.0f;
		//0 90 180 270 360
		//0 1   0		-1   0
		//1 0		-1	 0	 1
		mDirection.x = sinf(degrees);
		mDirection.y = cosf(degrees);
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
		auto action = node->runAction(EaseOut::create(RotateTo::create(mSpeed * 0.005, aAngle), 2.0f));
		action->setTag(static_cast<int>(eActionTag::ACTION_TAG_ROTATE));
		result = static_cast<ActionInterval*>(action)->getDuration();
	}
	return result;
}
NS_CC_END
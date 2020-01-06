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
		auto animationMove = dynamic_cast<Speed*>(node->getActionByTag(7));
		animationMove->setSpeed(1.0f);
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_MOVE));
		auto moveAction = node->runAction(
			RepeatForever::create(
					MoveBy::create(1.0f * 0.05f, Vec2(mSpeed * mDirection.x, mSpeed * mDirection.y) * 0.05f)
			)
		);
		moveAction->setTag(static_cast<int>(eActionTag::ACTION_MOVE));
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
		auto animationMove = dynamic_cast<Speed*>(node->getActionByTag(7));
		animationMove->setSpeed(0.0f);
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_MOVE));
	}
}
void IMovableObject::turnOn(float aAngle)
{
	auto node = getVisualNode();
	if ( node )
	{
		stop();
		int degrees = ((node->getRotation() + aAngle) * M_PI) / 180;
															   //0 90 180 270 360
		mDirection.x = sinf(degrees);//0 1   0	-1   0
		mDirection.y = cosf(degrees);//1 0	-1	 0	 1

		node->runAction(EaseOut::create(RotateBy::create(0.15f, aAngle), 2.0f));
	}
}
void IMovableObject::turnTo(float aAngle)
{
	auto node = getVisualNode();
	if ( node )
	{
		stop();
		int degrees = ((node->getRotation() + aAngle) * M_PI) / 180;
		//0 90 180 270 360
		//0 1   0		-1   0
		//1 0		-1	 0	 1
		mDirection.x = sinf(degrees);
		mDirection.y = cosf(degrees);

		node->runAction(EaseOut::create(RotateTo::create(mSpeed * 0.05, aAngle), 2.0f));
	}
}
NS_CC_END
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

IMovableObject::~IMovableObject()
{
}

void IMovableObject::build()
{
	IPhysicsObject::build();
	setMovable(true);
}
void IMovableObject::move()
{
	auto node = getVisualNode();
	if ( node )
	{
		auto moveAction = node->runAction(MoveBy::create(1.0f * 0.1f, Vec2(mSpeed * mDirection.x, mSpeed * mDirection.y) * 0.1f));
		moveAction->setTag(static_cast<int>(eActionTag::ACTION_MOVE));
	}
}
void IMovableObject::run()
{
	mIsRunning = true;
}
void IMovableObject::stop()
{
	mIsRunning = false;
	auto node = getVisualNode();
	if ( node )
	{
		node->stopActionByTag(static_cast<int>(eActionTag::ACTION_MOVE));
	}
}
void IMovableObject::turnOn(float aAngle)
{
	auto node = getVisualNode();
	if ( node )
	{
		//float degrees = node->getRotation();
		node->runAction(EaseOut::create(RotateBy::create(0.15f, aAngle), 2.0f));
	}
}
NS_CC_END
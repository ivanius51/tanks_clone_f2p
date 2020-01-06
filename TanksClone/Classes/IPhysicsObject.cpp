#include "IPhysicsObject.h"
NS_CC_BEGIN
IPhysicsObject::IPhysicsObject()
	:mCollisionType(eCollisionType::COLLISION_TYPE_RECT)
{
}

IPhysicsObject::~IPhysicsObject()
{
}

bool IPhysicsObject::build()
{
	setPassable(false);
	return true;
}

bool IPhysicsObject::isCollidedTo(const IGameObject* aOther)
{
	bool result = false;
	/*if (
		!isFlyable() && aOther->isPassable()
		&& !aOther->isFlyable()
		&& isDestroyable() 
		&& aOther->isDestroyable 
		)*/
	{
		auto thisNode = getVisualNode();
		auto otherNode = aOther->getVisualNode();

		if ( thisNode && otherNode )
		{
			const Rect thisPos = thisNode->getBoundingBox();
			const Rect OtherPos = otherNode->getBoundingBox();
			result = thisPos.intersectsRect(OtherPos);
		}
	}
	return result;
}
NS_CC_END
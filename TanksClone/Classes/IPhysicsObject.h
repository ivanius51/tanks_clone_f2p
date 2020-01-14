#ifndef _IPhysicsObject_H_
#define _IPhysicsObject_H_

#include "IGameObject.h"
#include "ObjectTypes.h"

NS_CC_BEGIN
class IPhysicsObject : public IGameObject
{
private:
	eCollisionType mCollisionType;
public:
	IPhysicsObject();
	virtual ~IPhysicsObject();

	virtual bool build() override;

	bool isCollidedTo(const IGameObject* aOther);
	cocos2d::Vec2 getCollidedVec(const IGameObject* aOther);
};
NS_CC_END
#endif
#ifndef IMovable_Object
#define IMovable_Object

#include "IPhysicsObject.h"
NS_CC_BEGIN
class IMovableObject : public IPhysicsObject
{
private:
	bool mIsRunning;
	float mSpeed;

	Vec2 mDirection;

public:
	IMovableObject();
	virtual ~IMovableObject();

	virtual void build() override;

	void move();
	void run();
	void stop();
	void turnOn(float aAngle);
};
NS_CC_END
#endif
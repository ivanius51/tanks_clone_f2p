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
protected:
	virtual void onMoveStart();
	virtual void onMoveEnd();

public:
	IMovableObject();
	virtual ~IMovableObject();

	virtual bool build() override;

	void move();
	void run();
	void stop();
	void turnOn(float aAngle);
	void turnTo(float aAngle);
};
NS_CC_END
#endif
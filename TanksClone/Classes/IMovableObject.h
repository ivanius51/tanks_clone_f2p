#ifndef _IMovableObject_H_
#define _IMovableObject_H_

#include "IPhysicsObject.h"
NS_CC_BEGIN
class IMovableObject : public IPhysicsObject
{
private:
	bool mIsRunning;
	bool mIsRotating;
	float mSpeed;

	Vec2 mDirection;
protected:
	virtual void onMoveStart();
	virtual void onMoveEnd();
	void updateDirrection(float aAngle = 0.0f);

	virtual void update(float aDelta);

public:
	IMovableObject();
	virtual ~IMovableObject();

	virtual bool build() override;

	void move();
	void run();
	void stop();
	float turnOn(float aAngle);
	float turnTo(float aAngle);

	virtual void onCommandReceived(GameCommand aCommand) override;
};
NS_CC_END
#endif
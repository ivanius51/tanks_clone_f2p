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
	IMovableObject::onCommandReceived(aCommand);
}

NS_CC_END

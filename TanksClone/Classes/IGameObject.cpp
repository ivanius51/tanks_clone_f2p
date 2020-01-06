#include "IGameObject.h"
NS_CC_BEGIN
void IGameObject::setHealth(int aHealth)
{
	mHealth = aHealth;
}

void IGameObject::setDamage(int aDamage)
{
	mDamage = aDamage;
}

void IGameObject::setMovable(bool aIsMovable)
{
	mMovable = aIsMovable;
}

void IGameObject::setDestroyable(bool aIsDestroyable)
{
	mDestroyable = aIsDestroyable;
}

void IGameObject::setPassable(bool aIsPassable)
{
	mPassable = aIsPassable;
}

void IGameObject::setFlyable(bool aIsFlyable)
{
	mFlyable = aIsFlyable;
}

void IGameObject::setType(eObjectType aType)
{
	mObjectType = aType;
}

void IGameObject::setPlayer(bool aIsPlayer)
{
	mIsPlayer = aIsPlayer;
}

const std::string & IGameObject::getName() const
{
	return mName;
}

void IGameObject::setName(const std::string& aName)
{
	mName = aName;
}

void IGameObject::setVisualNode(cocos2d::Node * aVisualNode)
{
	mVisualNode = aVisualNode;
	mVisualNode->setOnExitCallback(CC_CALLBACK_0(IGameObject::onDestroy, this));
}

void IGameObject::onDestroy()
{
	mOnDestroyCallback(this);
}

IGameObject::IGameObject()
	: mHealth(0)
	, mDamage(0)
	, mDestroyable(false)
	, mFlyable(true)
	, mMovable(false)
	, mObjectType(eObjectType::OBJECT_TYPE_UNDEFINED)
	, mPassable(true)
	, mVisualNode(nullptr)
	, mIsPlayer(false)
{
}

IGameObject::~IGameObject()
{
}

int IGameObject::getHealth() const
{
	return mHealth;
}

int IGameObject::getDamage() const
{
	return mDamage;
}

bool IGameObject::isMovable() const
{
	return mMovable;
}

bool IGameObject::isDestroyable() const
{
	return mDestroyable;
}

bool IGameObject::isPassable() const
{
	return mPassable;
}

bool IGameObject::isFlyable() const
{
	return mFlyable;
}

bool IGameObject::isPlayer() const
{
	return mIsPlayer;
}

eObjectType IGameObject::getType() const
{
	return mObjectType;
}

cocos2d::Node* IGameObject::getVisualNode() const
{
	return mVisualNode;
}

void IGameObject::setOnDestroyCallback(const std::function<void(IGameObject*)>& aOnDestroyCallback)
{
	mOnDestroyCallback = aOnDestroyCallback;
}

void IGameObject::onCommandReceived(GameCommand aCommand)
{

}
NS_CC_END
#ifndef IGame_Object
#define IGame_Object

#include "ObjectTypes.h"
#include "GameCommand.h"

#include "2d/CCNode.h"
NS_CC_BEGIN
class IGameObject
{
private:
	int mHealth;
	int mDamage;

	bool mMovable;
	bool mDestroyable;
	bool mPassable;
	bool mFlyable;

	eObjectType mObjectType;

	Node* mVisualNode;

protected:
	void setHealth(int aHealth);
	void setDamage(int aDamage);

	void setMovable(bool aIsMovable);
	void setDestroyable(bool aIsDestroyable);
	void setPassable(bool aIsPassable);
	void setFlyable(bool aIsFlyable);

	void setVisualNode(Node* aVisualNode);

public:
	IGameObject();
	virtual ~IGameObject();
	virtual void build() = 0;

	virtual int getHealth() const;
	virtual int getDamage() const;

	virtual bool isMovable() const;
	virtual bool isDestroyable() const;
	virtual bool isPassable() const;
	virtual bool isFlyable() const;

	Node* getVisualNode() const;

	virtual void onCommandReceived(GameCommand aCommand);
};
NS_CC_END
#endif//IGame_Object


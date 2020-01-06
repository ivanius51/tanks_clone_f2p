#ifndef IGame_Object
#define IGame_Object

#include "ObjectTypes.h"
#include "GameCommand.h"
#include "cocos2d.h"

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
	bool mIsPlayer;

	eObjectType mObjectType;
	std::string mName;
	Node* mVisualNode;

	std::function<void(IGameObject*)> mOnDestroyCallback;

protected:
	void setHealth(int aHealth);
	void setDamage(int aDamage);

	void setMovable(bool aIsMovable);
	void setDestroyable(bool aIsDestroyable);
	void setPassable(bool aIsPassable);
	void setFlyable(bool aIsFlyable);
	void setType(eObjectType aType);

	void setVisualNode(Node* aVisualNode);

	virtual void onDestroy();
	virtual bool build() = 0;

public:
	IGameObject();
	virtual ~IGameObject();

	virtual int getHealth() const;
	virtual int getDamage() const;

	virtual bool isMovable() const;
	virtual bool isDestroyable() const;
	virtual bool isPassable() const;
	virtual bool isFlyable() const;

	bool isPlayer() const;
	void setPlayer(bool aIsPlayer);
	const std::string& getName() const;
	void setName(const std::string& aName);

	eObjectType getType() const;
	Node* getVisualNode() const;
	void setOnDestroyCallback(const std::function<void(IGameObject*)>& aOnDestroyCallback);

	virtual void onCommandReceived(GameCommand aCommand);
};
NS_CC_END
#endif//IGame_Object


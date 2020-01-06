#ifndef _ObjectFactory
#define _ObjectFactory

#include "GameCommand.h"
#include "IGameObject.h"

NS_CC_BEGIN
class ObjectFactory
{
private:
	ObjectFactory();
	~ObjectFactory();
	ObjectFactory(const ObjectFactory&);
	ObjectFactory& operator=(const ObjectFactory&);

	std::map < eObjectType, std::vector<IGameObject*> > mObjects;
	void onObjectDestroy(IGameObject* aObject);

public:
	static ObjectFactory& getInstance();

	const std::vector<IGameObject*>* getObjectsByType(eObjectType aObjectType) const;
	const std::map < eObjectType, std::vector<IGameObject*> >  getObjects() const;

	IGameObject* create(eObjectType aType, ValueMap aData, Node* aParent);
	Node* create(eNodeType aType, ValueMap aData, Node* aParent);
};

#define FACTORY ObjectFactory::getInstance()

NS_CC_END
#endif

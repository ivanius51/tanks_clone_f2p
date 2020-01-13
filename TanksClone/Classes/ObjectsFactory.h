#ifndef _ObjectsFactory
#define _ObjectsFactory

#include "GameCommand.h"
#include "IGameObject.h"

NS_CC_BEGIN
class IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) = 0;
};

class ZOrderSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};	

class PositionSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};		 	

class PositionNormalizedSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};	

class ScaleSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};	

class NameSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};	

class TagSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};	

class TexturesSetter : public IParamsSetterStrategy
{
public:
	virtual bool execute(Value aData, Node* aNode) override;
};	

class ObjectsFactory
{
private:
	ObjectsFactory();
	~ObjectsFactory();

	void createParamsSetters();
	ObjectsFactory(const ObjectsFactory&);
	ObjectsFactory& operator=(const ObjectsFactory&);

	std::map < eObjectType, std::vector<IGameObject*> > mObjects;
	void onObjectDestroy(IGameObject* aObject);
	std::map<std::string, IParamsSetterStrategy*> paramsSetters;

public:
	static ObjectsFactory& getInstance();

	const std::vector<IGameObject*>* getObjectsByType(eObjectType aObjectType) const;
	const std::map < eObjectType, std::vector<IGameObject*> >  getObjects() const;

	IGameObject* create(eObjectType aType, const ValueMap& aData, Node* aParent);
	Node* create(eNodeType aType, const ValueMap& aData, Node* aParent);

	void setObjectParams(const ValueMap& aData, Node* aNode);
};

#define FACTORY ObjectsFactory::getInstance()

NS_CC_END
#endif

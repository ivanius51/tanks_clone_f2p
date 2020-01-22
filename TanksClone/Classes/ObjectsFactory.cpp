#include "ObjectsFactory.h"
#include "Tank.h"
#include "ui/UIImageView.h"

NS_CC_BEGIN
ObjectsFactory::ObjectsFactory()
{
	createParamsSetters();
}

ObjectsFactory::~ObjectsFactory()
{
	for ( auto objectsIt : mObjects )
	{
		for ( auto objectIt : objectsIt.second )
		{
			if ( objectIt )
			{
				CC_SAFE_DELETE(objectIt);
			}
		}
	}
	for ( auto& setterIt : paramsSetters )
	{
		if ( setterIt.second )
		{
			CC_SAFE_DELETE(setterIt.second);
		}
	}
}

void ObjectsFactory::createParamsSetters()
{
	paramsSetters["zorder"] = new ZOrderSetter();
	paramsSetters["position"] = new PositionSetter();
	paramsSetters["position_normalized"] = new PositionNormalizedSetter();
	paramsSetters["scale"] = new ScaleSetter();
	paramsSetters["name"] = new NameSetter();
	paramsSetters["tag"] = new TagSetter();
	paramsSetters["texture_path"] = new TexturesSetter();
}

void ObjectsFactory::onObjectDestroy(IGameObject* aObject)
{
	auto& objects = mObjects[aObject->getType()];
	auto findIt = std::find(objects.begin(), objects.end(), aObject);
	if ( findIt != objects.end() )
	{
		CC_SAFE_DELETE(*findIt);
		objects.erase(findIt);
	}
}

ObjectsFactory& ObjectsFactory::getInstance()
{
	static ObjectsFactory instance;
	return instance;
}

const std::vector<IGameObject*>* ObjectsFactory::getObjectsByType(eObjectType aObjectType) const
{
	const std::vector<IGameObject*>* result =  nullptr;
	auto objectsIt = mObjects.find(aObjectType);
	if ( objectsIt != mObjects.end() )
	{
		result = &objectsIt->second;
	}
	return result;
}

const std::map<eObjectType, std::vector<IGameObject*>> ObjectsFactory::getObjects() const
{
	return mObjects;
}

IGameObject* ObjectsFactory::create(eObjectType aType, const ValueMap& aData, Node* aParent)
{
	IGameObject* result = nullptr;
	Node* nodeObject = nullptr;

	auto findIt = aData.find("node_type");
	if ( findIt != aData.end() )
	{
		nodeObject = create(static_cast<eNodeType>(findIt->second.asInt()), aData, aParent);
	}

	if ( nodeObject )
	{
		switch ( aType )
		{
			case eObjectType::OBJECT_TYPE_VEHICLE:
			{
				eVehicleType vehicleType = eVehicleType::VEHICLE_TYPE_UNDEFINED;
				auto findIt = aData.find("type");
				if ( findIt != aData.end() )
				{
					vehicleType = static_cast<eVehicleType>(findIt->second.asInt());
				}
				switch ( vehicleType )
				{
					case eVehicleType::VEHICLE_TYPE_TANK:
						result = Tank::create(nodeObject);
						break;
					default:
						break;
				}
			}
				break;
			case eObjectType::OBJECT_TYPE_BULLET:
				break;
			case eObjectType::OBJECT_TYPE_WALL:
				break;
			case eObjectType::OBJECT_TYPE_GROUND:
				break;
			case eObjectType::OBJECT_TYPE_BONUS:
				break;
			default:
				break;
		}

		if ( result )
		{
			auto findIt = aData.find("name");
			if ( findIt != aData.end() )
			{
				result->setName(findIt->second.asString());
			}

			result->setOnDestroyCallback(CC_CALLBACK_1(ObjectsFactory::onObjectDestroy, this));
			mObjects[aType].push_back(result);
		}
		else
		{
			nodeObject->removeFromParent();
		}
	}
	return result;
}

Node* ObjectsFactory::create(eNodeType aType, const ValueMap& aData, Node* aParent)
{
	Node* result = nullptr;

	switch ( aType )
	{
		case eNodeType::NODE_TYPE_NODE:
			break;
		case eNodeType::NODE_TYPE_SPRITE:
		{
			result = Sprite::create();
		}
			break;
		case eNodeType::NODE_TYPE_IMAGE_VIEW:
			break;
		case eNodeType::NODE_TYPE_BUTTON:
			break;
		case eNodeType::NODE_TYPE_LABEL:
			break;
		default:
			break;
	}
	if ( result )
	{
		aParent->addChild(result);
		setObjectParams(aData, result);
	}
	return result;
}

void ObjectsFactory::setObjectParams(const ValueMap& aData, Node* aNode)
{
	for ( const auto& setterIt : paramsSetters )
	{
		auto findIt = aData.find(setterIt.first);
		if ( findIt != aData.end() )
		{
			if ( setterIt.second )
			{
				setterIt.second->execute(findIt->second, aNode);
			}
		}
	}
}

bool ZOrderSetter::execute(Value aData, Node* aNode)
{
	bool result = false;
	if ( aData.getType() == Value::Type::INTEGER )
	{
		aNode->setLocalZOrder(aData.asInt());
	}
	return result;
}

bool PositionSetter::execute(Value aData, Node* aNode)
{
	bool result = false;
	if ( aData.getType() == Value::Type::VECTOR )
	{
		auto posVec = aData.asValueVector();
		aNode->setPosition(posVec[0].asFloat(), posVec[1].asFloat());
		result = true;
	}
	return result;
}

bool PositionNormalizedSetter::execute(Value aData, Node * aNode)
{
	return false;
}

bool ScaleSetter::execute(Value aData, Node* aNode)
{
	bool result = false;
	if ( aData.getType() == Value::Type::VECTOR )
	{
		auto posVec = aData.asValueVector();
		aNode->setScaleX(posVec[0].asFloat());
		aNode->setScaleY(posVec[1].asFloat());
		result = true;
	}
	else if ( aData.getType() == Value::Type::FLOAT )
	{
		aNode->setScale(aData.asFloat());
		result = true;
	}
	return result;
}

bool NameSetter::execute(Value aData, Node* aNode)
{
	bool result = false;
	if ( aData.getType() == Value::Type::STRING )
	{
		aNode->setName(aData.asString());
	}
	return result;
}

bool TagSetter::execute(Value aData, Node* aNode)
{
	bool result = false;
	if ( aData.getType() == Value::Type::INTEGER )
	{
		aNode->setTag(aData.asInt());
		result = true;
	}
	return result;
}

bool TexturesSetter::execute(Value aData, Node* aNode)
{
	bool result = false;
	if ( aData.getType() == Value::Type::STRING )
	{
		auto path = aData.asString();
		auto spritecache = SpriteFrameCache::getInstance();
		auto imageView = dynamic_cast<ui::ImageView*>(aNode);
		auto sprite = dynamic_cast<Sprite*>(aNode);
		auto spriteFrame = SpriteFrameCache::getInstance()->getSpriteFrameByName(path);
		if ( spriteFrame )
		{
			if ( imageView )
			{
				imageView->loadTexture(spriteFrame);
			}
			else if ( sprite )
			{
				sprite->setSpriteFrame(spriteFrame);
			}
		}
		else
		{
			if ( imageView )
			{
				imageView->loadTexture(path);
			}
			else if ( sprite )
			{
				sprite->setTexture(path);
			}
		}	 
	}
	return result;
}

NS_CC_END

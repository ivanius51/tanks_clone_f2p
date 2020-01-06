#include "ObjectFactory.h"
#include "Tank.h"

NS_CC_BEGIN
ObjectFactory::ObjectFactory()
{
	
}

ObjectFactory::~ObjectFactory()
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
}

void ObjectFactory::onObjectDestroy(IGameObject* aObject)
{
	auto& objects = mObjects[aObject->getType()];
	auto findIt = std::find(objects.begin(), objects.end(), aObject);
	if ( findIt != objects.end() )
	{
		CC_SAFE_DELETE(*findIt);
		objects.erase(findIt);
	}
}

ObjectFactory& ObjectFactory::getInstance()
{
	static ObjectFactory instance;
	return instance;
}

const std::vector<IGameObject*>* ObjectFactory::getObjectsByType(eObjectType aObjectType) const
{
	const std::vector<IGameObject*>* result =  nullptr;
	auto objectsIt = mObjects.find(aObjectType);
	if ( objectsIt != mObjects.end() )
	{
		result = &objectsIt->second;
	}
	return result;
}

const std::map<eObjectType, std::vector<IGameObject*>> ObjectFactory::getObjects() const
{
	return mObjects;
}

IGameObject* ObjectFactory::create(eObjectType aType, ValueMap aData, Node* aParent)
{
	IGameObject* result = nullptr;
	
	auto nodeType = static_cast<eNodeType>(aData["node_type"].asInt());
	Node* nodeObject = create(nodeType, aData, aParent);

	if ( nodeObject )
	{
		switch ( aType )
		{
			case eObjectType::OBJECT_TYPE_VEHICLE:
			{
				auto vehicleType = static_cast<eVehicleType>(aData["type"].asInt());
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

			result->setOnDestroyCallback(CC_CALLBACK_1(ObjectFactory::onObjectDestroy, this));
			mObjects[aType].push_back(result);
		}
	}
	return result;
}

Node* ObjectFactory::create(eNodeType aType, ValueMap aData, Node* aParent)
{
	Node* result = nullptr;

	std::string filePath;
	auto findIt = aData.find("path");
	if ( findIt != aData.end() )
	{
		filePath = findIt->second.asString();
	}

	switch ( aType )
	{
		case eNodeType::NODE_TYPE_NODE:
			break;
		case eNodeType::NODE_TYPE_SPRITE:
		{
			auto spritecache = SpriteFrameCache::getInstance();
			auto newspriteFrame = spritecache->getSpriteFrameByName(filePath);
			if ( newspriteFrame )
			{
				result = Sprite::createWithSpriteFrame(newspriteFrame);
			}
			else
			{
				result = Sprite::create(filePath);
			}
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

		findIt = aData.find("zorder");
		if ( findIt != aData.end() )
		{
			result->setLocalZOrder(findIt->second.asInt());
		}

		findIt = aData.find("name");
		if ( findIt != aData.end() )
		{
			result->setName(findIt->second.asString());
		}
		
		findIt = aData.find("tag");
		if ( findIt != aData.end() )
		{
			result->setTag(findIt->second.asInt());
		}

		findIt = aData.find("positon");
		if ( findIt != aData.end() )
		{
			auto posVec = findIt->second.asValueVector();
			float x = posVec[0].asFloat();
			float y = posVec[1].asFloat();
			if ( fabs(x) <= 1.0f
				&& fabs(y) <= 1.0f )
			{
				result->setPositionNormalized(Vec2(x, y));
			}
			else
			{
				result->setPosition(x, y);
			}
		}

		findIt = aData.find("scale");
		if ( findIt != aData.end() )
		{
			auto scaleVec = findIt->second.asValueVector();
			float x = scaleVec[0].asFloat();
			float y = scaleVec[1].asFloat();
			result->setScale(x, y);
		}
	}
	return result;
}



NS_CC_END
#include "game/ObjectsFactory.h"

#include <algorithm>
#include <string_view>
#include "ui/UIImageView.h"
#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "game/objects/Bonus.h"
#include "game/objects/Bullet.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"

namespace tanks
{

namespace
{

constexpr float DefaultAnimationFrameDelay = 0.15f;

// Applies one named parameter of the object description to a Node.
// Returns true if the value was applied.
// Setters get the node together with the type the factory created it as, so no setter has to guess the type.
using ParamSetter = bool (*)(const cocos2d::Value& InData, cocos2d::Node* InNode, NodeType InNodeType);

bool IsNumber(const cocos2d::Value& InValue)
{
	switch ( InValue.getType() )
	{
		case cocos2d::Value::Type::INTEGER:
		case cocos2d::Value::Type::UNSIGNED:
		case cocos2d::Value::Type::FLOAT:
		case cocos2d::Value::Type::DOUBLE:
			return true;
		default:
			return false;
	}
}

bool SetZOrder(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::INTEGER )
	{
		return false;
	}
	InNode->setLocalZOrder(InData.asInt());
	return true;
}

bool SetPosition(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::VECTOR || InData.asValueVector().size() < 2 )
	{
		return false;
	}
	InNode->setPosition(DataConverter::ValueToVec2(InData));
	return true;
}

// Position as a fraction of the visible area: [0.5, 0.5] is the screen center.
bool SetPositionNormalized(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::VECTOR || InData.asValueVector().size() < 2 )
	{
		return false;
	}

	const cocos2d::Vec2 Fraction = DataConverter::ValueToVec2(InData);
	const cocos2d::Size VisibleSize = cocos2d::Director::getInstance()->getVisibleSize();
	const cocos2d::Vec2 CurrentOrigin = cocos2d::Director::getInstance()->getVisibleOrigin();
	InNode->setPosition(CurrentOrigin.x + VisibleSize.width * Fraction.x, CurrentOrigin.y + VisibleSize.height * Fraction.y);
	return true;
}

bool SetScale(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() == cocos2d::Value::Type::VECTOR )
	{
		if ( InData.asValueVector().size() < 2 )
		{
			return false;
		}
		const cocos2d::Vec2 Scale = DataConverter::ValueToVec2(InData);
		InNode->setScaleX(Scale.x);
		InNode->setScaleY(Scale.y);
		return true;
	}
	if ( IsNumber(InData) )
	{
		InNode->setScale(InData.asFloat());
		return true;
	}
	return false;
}

bool SetName(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::STRING )
	{
		return false;
	}
	InNode->setName(InData.asString());
	return true;
}

bool SetTag(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::INTEGER )
	{
		return false;
	}
	InNode->setTag(InData.asInt());
	return true;
}

bool SetColor(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::VECTOR || InData.asValueVector().size() < 3 )
	{
		return false;
	}
	InNode->setColor(DataConverter::ValueToColor3B(InData));
	return true;
}

bool SetRotation(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( !IsNumber(InData) )
	{
		return false;
	}
	InNode->setRotation(InData.asFloat());
	return true;
}

bool SetTexture(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::STRING )
	{
		return false;
	}

	const std::string Path = InData.asString();
	auto CurrentSpriteFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Path);

	switch ( InNodeType )
	{
		case NodeType::ImageView:
		{
			auto CurrentImageView = static_cast<cocos2d::ui::ImageView*>(InNode);
			if ( CurrentSpriteFrame )
			{
				CurrentImageView->loadTexture(CurrentSpriteFrame);
			}
			else
			{
				CurrentImageView->loadTexture(Path);
			}
			return true;
		}
		case NodeType::Sprite:
		{
			auto CurrentSprite = static_cast<cocos2d::Sprite*>(InNode);
			if ( CurrentSpriteFrame )
			{
				CurrentSprite->setSpriteFrame(CurrentSpriteFrame);
			}
			else
			{
				// Not a frame from the sprite cache: treat it as a texture file path.
				CurrentSprite->setTexture(Path);
			}
			return true;
		}
		default:
			break;
	}
	return false;
}

// {"name": "tank_anim_%d", "frames_count": 7, "delay": 0.15}
// Frames are numbered from 1. The animation is created paused (speed 0):
// the movable object speeds it up while moving.
bool SetMoveAnimation(const cocos2d::Value& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::MAP )
	{
		return false;
	}

	const cocos2d::ValueMap& Description = InData.asValueMap();
	auto NameIt = Description.find("name");
	auto CountIt = Description.find("frames_count");
	if ( NameIt == Description.end() || CountIt == Description.end() )
	{
		return false;
	}

	auto DelayIt = Description.find("delay");
	const float CurrentDelay = DelayIt != Description.end() ? DelayIt->second.asFloat() : DefaultAnimationFrameDelay;

	const std::string Pattern = NameIt->second.asString();
	const int CurrentCount = CountIt->second.asInt();

	auto SpriteCache = cocos2d::SpriteFrameCache::getInstance();
	auto CurrentAnimation = cocos2d::Animation::create();
	for ( int Index = 1; Index <= CurrentCount; ++Index )
	{
		auto CurrentFrame = SpriteCache->getSpriteFrameByName(cocos2d::StringUtils::format(Pattern.c_str(), Index));
		if ( CurrentFrame )
		{
			CurrentAnimation->addSpriteFrame(CurrentFrame);
		}
	}
	if ( CurrentAnimation->getFrames().empty() )
	{
		return false;
	}
	CurrentAnimation->setDelayPerUnit(CurrentDelay);
	CurrentAnimation->setLoops(-1);

	auto AnimationAction = cocos2d::Speed::create(cocos2d::Animate::create(CurrentAnimation), 0.0f);
	AnimationAction->setTag(ToTag(ActionTag::Animation));
	InNode->runAction(AnimationAction);
	return true;
}

const std::vector<std::pair<const char*, ParamSetter>>& ParamSetters()
{
	static const std::vector<std::pair<const char*, ParamSetter>> SETTERS = {
		{ "zorder", &SetZOrder },
		{ "position", &SetPosition },
		{ "position_normalized", &SetPositionNormalized },
		{ "scale", &SetScale },
		{ "name", &SetName },
		{ "tag", &SetTag },
		{ "texture_path", &SetTexture },
		{ "color", &SetColor },
		{ "rotation", &SetRotation },
		{ "move_animation", &SetMoveAnimation }
	};
	return SETTERS;
}

} // namespace

ObjectsFactory& ObjectsFactory::GetInstance()
{
	static ObjectsFactory Instance;
	return Instance;
}

GameObject* ObjectsFactory::CreateFromConfig(const std::string& InConfigId, cocos2d::Node* const InParent, const cocos2d::ValueMap& InOverrides)
{
	const ObjectConfig& Config = ConfigReader::GetInstance().GetObjectData(InConfigId);
	if ( Config.Values.empty() )
	{
		return nullptr;
	}

	cocos2d::ValueMap ConfigData = Config.Values;
	for ( const auto& OverrideIt : InOverrides )
	{
		ConfigData[OverrideIt.first] = OverrideIt.second;
	}

	auto TypeIt = ConfigData.find("object_type");
	if ( TypeIt == ConfigData.end() )
	{
		CCLOGERROR("ObjectsFactory: no object_type in %s", InConfigId.c_str());
		return nullptr;
	}
	return Create(ParseObjectType(TypeIt->second), ConfigData, InParent);
}

GameObject* ObjectsFactory::Create(const ObjectType InType, const cocos2d::ValueMap& InData, cocos2d::Node* const InParent)
{
	if ( !InParent )
	{
		return nullptr;
	}

	auto NodeTypeIt = InData.find("node_type");
	if ( NodeTypeIt == InData.end() )
	{
		return nullptr;
	}

	cocos2d::Node* CurrentVisualNode = CreateNode(ParseNodeType(NodeTypeIt->second), InData, InParent);
	if ( !CurrentVisualNode )
	{
		return nullptr;
	}

	std::unique_ptr<GameObject> Object;
	switch ( InType )
	{
		case ObjectType::Vehicle:
			Object = CreateVehicle(InData, CurrentVisualNode);
			break;
		case ObjectType::Bullet:
			Object = Bullet::Create(CurrentVisualNode);
			break;
		case ObjectType::Wall:
			Object = Wall::Create(CurrentVisualNode);
			break;
		case ObjectType::Bonus:
			Object = Bonus::Create(CurrentVisualNode);
			break;
		default: // TODO: ground
			break;
	}

	if ( !Object )
	{
		CurrentVisualNode->removeFromParent();
		return nullptr;
	}

	Object->Configure(InData);

	auto NameIt = InData.find("name");
	if ( NameIt != InData.end() )
	{
		Object->SetName(NameIt->second.asString());
	}
	Object->SetOnDestroyCallback([this](GameObject* const InObject) { OnObjectDestroy(InObject); });

	GameObject* Result = Object.get();
	Objects[InType].push_back(std::move(Object));
	return Result;
}

std::unique_ptr<GameObject> ObjectsFactory::CreateVehicle(const cocos2d::ValueMap& InData, cocos2d::Node* const InNode)
{
	auto TypeIt = InData.find("type");
	if ( TypeIt == InData.end() )
	{
		return nullptr;
	}

	switch ( ParseVehicleType(TypeIt->second) )
	{
		case VehicleType::Tank:
			return Tank::Create(InNode);
		default:
			return nullptr;
	}
}

cocos2d::Node* ObjectsFactory::CreateNode(const NodeType InType, const cocos2d::ValueMap& InData, cocos2d::Node* const InParent)
{
	cocos2d::Node* Result = nullptr;

	switch ( InType )
	{
		case NodeType::Sprite:
			Result = cocos2d::Sprite::create();
			break;
		default: // TODO: node, image view, button, label
			break;
	}

	if ( Result && InParent )
	{
		InParent->addChild(Result);
		SetObjectParams(InData, Result, InType);
	}
	return Result;
}

GameObject* ObjectsFactory::FindObject(std::string_view InName, const ObjectType InType) const
{
	for ( const auto& TypeIt : Objects )
	{
		if ( InType != ObjectType::Undefined && TypeIt.first != InType )
		{
			continue;
		}
		for ( const auto& Object : TypeIt.second )
		{
			if ( Object->GetName() == InName )
			{
				return Object.get();
			}
		}
	}
	return nullptr;
}

void ObjectsFactory::ForEachObject(const ObjectType InType, const std::function<void(GameObject&)>& InAction) const
{
	for ( const auto& TypeIt : Objects )
	{
		if ( InType != ObjectType::Undefined && TypeIt.first != InType )
		{
			continue;
		}
		for ( const auto& Object : TypeIt.second )
		{
			InAction(*Object);
		}
	}
}

GameObject* ObjectsFactory::FindObjectIf(const std::function<bool(GameObject&)>& InPredicate) const
{
	for ( const auto& TypeIt : Objects )
	{
		for ( const auto& Object : TypeIt.second )
		{
			if ( !Object->IsDestroying() && InPredicate(*Object) )
			{
				return Object.get();
			}
		}
	}
	return nullptr;
}

void ObjectsFactory::SetObjectParams(const cocos2d::ValueMap& InData, cocos2d::Node* const InNode, const NodeType InNodeType)
{
	for ( const auto& Setter : ParamSetters() )
	{
		auto FindIt = InData.find(Setter.first);
		if ( FindIt != InData.end() )
		{
			Setter.second(FindIt->second, InNode, InNodeType);
		}
	}
}

void ObjectsFactory::OnObjectDestroy(GameObject* const InObject)
{
	auto& CurrentObjects = Objects[InObject->GetType()];
	CurrentObjects.erase(
		std::remove_if(CurrentObjects.begin(), CurrentObjects.end(),
			[InObject](const std::unique_ptr<GameObject>& InItem) { return InItem.get() == InObject; }),
		CurrentObjects.end());
}

} // namespace tanks

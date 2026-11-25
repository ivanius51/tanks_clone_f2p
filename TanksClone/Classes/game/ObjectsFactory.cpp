#include "game/ObjectsFactory.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <vector>

#include "ui/UIImageView.h"

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/objects/Bonus.h"
#include "game/objects/Bullet.h"
#include "game/objects/Tank.h"
#include "game/objects/TankLoader.h"
#include "game/objects/Wall.h"
#include "services/ConfigRefs.h"
#include "services/ResourceManager.h"

namespace game
{

namespace
{

constexpr float DefaultAnimationFrameDelay = 0.15f;

// Applies one named parameter of the object description to a Node.
// Returns true if the value was applied.
// Setters get the node together with the type the factory created it as, so no setter has to guess the type.
using ParamSetter = bool (*)(const cocos2d::Value& InData, cocos2d::Node* InNode, ENodeType InNodeType);

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

bool SetZOrder(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::INTEGER )
	{
		return false;
	}
	InNode->setLocalZOrder(InData.asInt());
	return true;
}

bool SetPosition(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::VECTOR || InData.asValueVector().size() < 2 )
	{
		return false;
	}
	InNode->setPosition(DataConverter::ValueToVec2(InData));
	return true;
}

// Position as a fraction of the visible area: [0.5, 0.5] is the screen center.
bool SetPositionNormalized(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
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

bool SetScale(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
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

bool SetName(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::STRING )
	{
		return false;
	}
	InNode->setName(InData.asString());
	return true;
}

bool SetTag(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::INTEGER )
	{
		return false;
	}
	InNode->setTag(InData.asInt());
	return true;
}

bool SetColor(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::VECTOR || InData.asValueVector().size() < 3 )
	{
		return false;
	}
	InNode->setColor(DataConverter::ValueToColor3B(InData));
	return true;
}

bool SetRotation(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( !IsNumber(InData) )
	{
		return false;
	}
	InNode->setRotation(InData.asFloat());
	return true;
}

const std::string HullSuffix = "_hull";
const std::string TurretSuffix = "_turret";
const std::string TurretNodeName = "turret";
constexpr int TurretZ = 10; // the top layer: above the hull and every overlay of TankLook
constexpr int TrackZ = -1;
const std::array<std::string, 2> TrackSides = { "left", "right" };
const std::string TrackNodePrefix = "track_";
const std::string MoveFramePattern = "anim_%d";

std::string TrackNodeName(const std::string& InSide)
{
	return TrackNodePrefix + InSide;
}

bool HasTrackLayers(const cocos2d::Node* const InNode)
{
	return InNode->getChildByName(TrackNodeName(TrackSides.front())) != nullptr;
}

void AddTrackLayers(cocos2d::Sprite* const InHull, const std::string& InPath)
{
	cocos2d::SpriteFrameCache* const FrameCache = cocos2d::SpriteFrameCache::getInstance();
	for ( const std::string& Side : TrackSides )
	{
		InHull->removeChildByName(TrackNodeName(Side));
		cocos2d::SpriteFrame* const FirstFrame = FrameCache->getSpriteFrameByName(InPath + "_" + TrackNodeName(Side) + "_1");
		if ( !FirstFrame )
		{
			continue;
		}
		cocos2d::Sprite* const Track = cocos2d::Sprite::createWithSpriteFrame(FirstFrame);
		Track->setName(TrackNodeName(Side));
		Track->setAnchorPoint(cocos2d::Vec2::ZERO);
		InHull->addChild(Track, TrackZ);
	}
}

cocos2d::Action* CreatePausedLoop(const std::vector<cocos2d::SpriteFrame*>& InFrames, const float InDelay)
{
	cocos2d::Animation* const CurrentAnimation = cocos2d::Animation::create();
	for ( cocos2d::SpriteFrame* const Frame : InFrames )
	{
		CurrentAnimation->addSpriteFrame(Frame);
	}
	CurrentAnimation->setDelayPerUnit(InDelay);
	CurrentAnimation->setLoops(std::numeric_limits<unsigned int>::max());
	cocos2d::Action* const Result = cocos2d::Speed::create(cocos2d::Animate::create(CurrentAnimation), 0.0f);
	Result->setTag(ToTag(EActionTag::Animation));
	return Result;
}

// Each track layer runs its own frames: the move pattern with "anim_%d" swapped for "track_<side>_%d".
bool SetTrackAnimations(cocos2d::Node* const InHull, const std::string& InMovePattern, const float InDelay)
{
	const size_t PatternAt = InMovePattern.find(MoveFramePattern);
	if ( PatternAt == std::string::npos )
	{
		return false;
	}
	cocos2d::SpriteFrameCache* const FrameCache = cocos2d::SpriteFrameCache::getInstance();
	for ( const std::string& Side : TrackSides )
	{
		cocos2d::Node* const Track = InHull->getChildByName(TrackNodeName(Side));
		if ( !Track )
		{
			continue;
		}
		std::string Pattern = InMovePattern;
		Pattern.replace(PatternAt, MoveFramePattern.size(), TrackNodeName(Side) + "_%d");
		std::vector<cocos2d::SpriteFrame*> Frames;
		while ( cocos2d::SpriteFrame* const Frame = FrameCache->getSpriteFrameByName(cocos2d::StringUtils::format(Pattern.c_str(), static_cast<int>(Frames.size()) + 1)) )
		{
			Frames.push_back(Frame);
		}
		if ( !Frames.empty() )
		{
			Track->runAction(CreatePausedLoop(Frames, InDelay));
		}
	}
	return true;
}

bool SetTexture(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
{
	if ( InData.getType() != cocos2d::Value::Type::STRING )
	{
		return false;
	}

	const std::string Path = InData.asString();
	cocos2d::SpriteFrame* CurrentSpriteFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Path);
	if ( !CurrentSpriteFrame && !cocos2d::FileUtils::getInstance()->isFileExist(Path) )
	{
		CurrentSpriteFrame = GameContext::Get<ResourceManager>().FindFrame(Path);
	}

	switch ( InNodeType )
	{
		case ENodeType::ImageView:
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
		case ENodeType::Sprite:
		{
			auto CurrentSprite = static_cast<cocos2d::Sprite*>(InNode);
			// The atlas is loaded once the main frame is found; asking the cache directly keeps objects without layers quiet.
			cocos2d::SpriteFrameCache* const FrameCache = cocos2d::SpriteFrameCache::getInstance();
			cocos2d::SpriteFrame* const HullFrame = CurrentSpriteFrame ? FrameCache->getSpriteFrameByName(Path + HullSuffix) : nullptr;
			cocos2d::SpriteFrame* const TurretFrame = HullFrame ? FrameCache->getSpriteFrameByName(Path + TurretSuffix) : nullptr;
			if ( HullFrame && TurretFrame )
			{
				// A tank drawn in two layers: the hull (which animates) and the turret above it, so the turret can be animated on its own.
				CurrentSprite->setSpriteFrame(HullFrame);
				CurrentSprite->removeChildByName(TurretNodeName);
				cocos2d::Sprite* const Turret = cocos2d::Sprite::createWithSpriteFrame(TurretFrame);
				Turret->setName(TurretNodeName);
				Turret->setAnchorPoint(cocos2d::Vec2::ZERO);
				CurrentSprite->addChild(Turret, TurretZ);
				AddTrackLayers(CurrentSprite, Path);
				CurrentSprite->setCascadeOpacityEnabled(true);
				CurrentSprite->setCascadeColorEnabled(true);
			}
			else if ( CurrentSpriteFrame )
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
bool SetMoveAnimation(const cocos2d::Value& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
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
	if ( HasTrackLayers(InNode) )
	{
		return SetTrackAnimations(InNode, Pattern, CurrentDelay);
	}

	ResourceManager& Resources = GameContext::Get<ResourceManager>();
	std::vector<cocos2d::SpriteFrame*> Frames;
	for ( int Index = 1; Index <= CurrentCount; ++Index )
	{
		if ( cocos2d::SpriteFrame* const CurrentFrame = Resources.FindFrame(cocos2d::StringUtils::format(Pattern.c_str(), Index)) )
		{
			Frames.push_back(CurrentFrame);
		}
	}
	if ( Frames.empty() )
	{
		return false;
	}
	InNode->runAction(CreatePausedLoop(Frames, CurrentDelay));
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

// "move_animation": "vfx.<id>" names an effect of views/effects.json; its frames, count and delay become the animation.
void ObjectsFactory::ResolveMoveAnimation(cocos2d::ValueMap& InOutValues) const
{
	constexpr std::string_view VfxPrefix = "vfx.";
	const auto AnimationIt = InOutValues.find("move_animation");
	if ( AnimationIt == InOutValues.end() || AnimationIt->second.getType() != cocos2d::Value::Type::STRING )
	{
		return;
	}
	const std::string Reference = AnimationIt->second.asString();
	const cocos2d::ValueMap& Effects = GameContext::Get<ConfigReader>().GetObjectData(config_refs::ResolveId(ids::configs::Effects)).Values;
	const auto EffectIt = Reference.compare(0, VfxPrefix.size(), VfxPrefix) == 0 ? Effects.find(Reference.substr(VfxPrefix.size())) : Effects.end();
	if ( EffectIt == Effects.end() )
	{
		CCLOGERROR("ObjectsFactory: unknown move_animation '%s'", Reference.c_str());
		InOutValues.erase(AnimationIt);
		return;
	}
	const cocos2d::ValueMap& Effect = EffectIt->second.asValueMap();
	cocos2d::ValueMap Animation;
	Animation["name"] = DataConverter::GetString(Effect, "frames", "");
	Animation["frames_count"] = DataConverter::GetInt(Effect, "count", 0);
	Animation["delay"] = DataConverter::GetFloat(Effect, "delay", DefaultAnimationFrameDelay);
	InOutValues["move_animation"] = Animation;
}

cocos2d::ValueMap ObjectsFactory::ReadValues(const std::string_view InConfigId) const
{
	const ConfigReader& Reader = GameContext::Get<ConfigReader>();
	cocos2d::ValueMap Result = Reader.GetObjectData(InConfigId).Values;
	for ( const char* const PartKey : { "look", "balance" } )
	{
		const auto PartIt = Result.find(PartKey);
		if ( PartIt == Result.end() || PartIt->second.getType() != cocos2d::Value::Type::STRING )
		{
			continue;
		}
		const FPersistentString PartPath = config_refs::ResolveName(PartIt->second.asString());
		for ( const auto& Entry : Reader.GetObjectData(PartPath).Values )
		{
			Result.emplace(Entry.first, Entry.second);
		}
	}
	ResolveMoveAnimation(Result);
	return Result;
}

FGameObjectHandle ObjectsFactory::CreateFromConfig(const std::string_view InConfigId, cocos2d::Node* const InParent, const cocos2d::ValueMap& InOverrides)
{
	cocos2d::ValueMap ConfigData = ReadValues(InConfigId);
	if ( ConfigData.empty() )
	{
		return nullptr;
	}

	for ( const auto& OverrideIt : InOverrides )
	{
		ConfigData[OverrideIt.first] = OverrideIt.second;
	}

	auto TypeIt = ConfigData.find("object_type");
	if ( TypeIt == ConfigData.end() )
	{
		CCLOGERROR("ObjectsFactory: no object_type in %.*s", static_cast<int>(InConfigId.size()), InConfigId.data());
		return nullptr;
	}
	FGameObjectHandle Result = Create(ParseObjectType(TypeIt->second), ConfigData, InParent);
	if ( Tank* const CreatedTank = ObjectCast<Tank>(Result.Get()) )
	{
		CreatedTank->ApplyDef(TankLoader::ReadTank(GameContext::Get<ConfigReader>().GetConfig(InConfigId)));
	}
	return Result;
}

FGameObjectHandle ObjectsFactory::Create(const EObjectType InType, const cocos2d::ValueMap& InData, cocos2d::Node* const InParent)
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

	FGameObjectHandle Object;
	switch (InType)
	{
		case EObjectType::Vehicle:
		{	
			Object = CreateVehicle(InData, CurrentVisualNode);
			break;
		}
		case EObjectType::Bullet:
		{
			Object = Bullet::Create(CurrentVisualNode);
			break;
		}
		case EObjectType::Wall:
		{
			Object = Wall::Create(CurrentVisualNode);
			break;
		}
		case EObjectType::Bonus:
		{
			Object = Bonus::Create(CurrentVisualNode);
			break;
		}
		default: // TODO: ground
			break;
	}

	if (!Object)
	{
		CurrentVisualNode->removeFromParent();
		return nullptr;
	}

	Object->Configure(InData);

	auto NameIt = InData.find("name");
	if (NameIt != InData.end())
	{
		Object->SetName(NameIt->second.asString());
	}
	Object->SetOnDestroyCallback([this](GameObject* const InObject) { OnObjectDestroy(InObject); });

	Objects[InType].EmplaceBack(Object);
	return Object;
}

FGameObjectHandle ObjectsFactory::CreateVehicle(const cocos2d::ValueMap& InData, cocos2d::Node* const InNode)
{
	auto TypeIt = InData.find("type");
	if ( TypeIt == InData.end() )
	{
		return nullptr;
	}

	switch ( ParseVehicleType(TypeIt->second) )
	{
		case EVehicleType::Tank:
		{	
			return Tank::Create(InNode);
		}
		default:
			return nullptr;
	}
}

cocos2d::Node* ObjectsFactory::CreateNode(const ENodeType InType, const cocos2d::ValueMap& InData, cocos2d::Node* const InParent)
{
	cocos2d::Node* Result = nullptr;

	switch ( InType )
	{
		case ENodeType::Sprite:
		{	
			Result = cocos2d::Sprite::create();
			break;
		}
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

FGameObjectHandle ObjectsFactory::FindObject(std::string_view InName, const EObjectType InType) const
{
	for ( const auto& TypeIt : Objects )
	{
		if ( InType != EObjectType::Undefined && TypeIt.Key != InType )
		{
			continue;
		}
		for ( const auto& Object : TypeIt.Value )
		{
			if ( Object->GetName() == InName )
			{
				return Object;
			}
		}
	}
	return nullptr;
}

void ObjectsFactory::ForEachObject(const EObjectType InType, const TFunctionRef<void(const FGameObjectHandle&)> InAction) const
{
	for ( const auto& TypeIt : Objects )
	{
		if ( InType != EObjectType::Undefined && TypeIt.Key != InType )
		{
			continue;
		}
		for ( const auto& Object : TypeIt.Value )
		{
			InAction(Object);
		}
	}
}

FGameObjectHandle ObjectsFactory::FindObjectIf(const TFunctionRef<bool(const FGameObjectHandle&)> InPredicate) const
{
	for ( const auto& TypeIt : Objects )
	{
		for ( const auto& Object : TypeIt.Value )
		{
			if ( !Object->IsDestroying() && InPredicate(Object) )
			{
				return Object;
			}
		}
	}
	return nullptr;
}

void ObjectsFactory::SetObjectParams(const cocos2d::ValueMap& InData, cocos2d::Node* const InNode, const ENodeType InNodeType)
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
	TArray<FGameObjectHandle>& CurrentObjects = Objects[InObject->GetType()];
	CurrentObjects.RemoveIf([InObject](const FGameObjectHandle& InItem) { return InItem.Get() == InObject; });
}

} // namespace game

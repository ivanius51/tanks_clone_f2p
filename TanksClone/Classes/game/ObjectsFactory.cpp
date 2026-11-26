#include "game/ObjectsFactory.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string>

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
#include "model/JsonValue.h"
#include "services/ConfigRefs.h"
#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game
{

	namespace
	{

		constexpr float DefaultAnimationFrameDelay = 0.15f;

		// Applies one named parameter of the object description to a Node.
		// Returns true if the value was applied.
		// Setters get the node together with the type the factory created it as, so no setter has to guess the type.
		using ParamSetter = bool (*)(const model::JsonValue& InData, cocos2d::Node* InNode, ENodeType InNodeType);

		bool IsNumber(const model::JsonValue& InValue)
		{
			return InValue.IsNumber();
		}

		bool SetZOrder(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (InData.GetType() != model::JsonValue::Type::Int)
			{
				return false;
			}
			InNode->setLocalZOrder(InData.AsInt());
			return true;
		}

		bool SetPosition(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!InData.IsArray() || InData.GetArray().Size() < 2)
			{
				return false;
			}
			InNode->setPosition(DataConverter::ValueToVec2(InData));
			return true;
		}

		// Position as a fraction of the visible area: [0.5, 0.5] is the screen center.
		bool SetPositionNormalized(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!InData.IsArray() || InData.GetArray().Size() < 2)
			{
				return false;
			}

			const cocos2d::Vec2 Fraction = DataConverter::ValueToVec2(InData);
			const cocos2d::Size VisibleSize = cocos2d::Director::getInstance()->getVisibleSize();
			const cocos2d::Vec2 CurrentOrigin = cocos2d::Director::getInstance()->getVisibleOrigin();
			InNode->setPosition(CurrentOrigin.x + VisibleSize.width * Fraction.x, CurrentOrigin.y + VisibleSize.height * Fraction.y);
			return true;
		}

		bool SetScale(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (InData.IsArray())
			{
				if (InData.GetArray().Size() < 2)
				{
					return false;
				}
				const cocos2d::Vec2 Scale = DataConverter::ValueToVec2(InData);
				InNode->setScaleX(Scale.x);
				InNode->setScaleY(Scale.y);
				return true;
			}
			if (IsNumber(InData))
			{
				InNode->setScale(InData.AsFloat());
				return true;
			}
			return false;
		}

		bool SetName(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!InData.IsString())
			{
				return false;
			}
			InNode->setName(InData.AsString());
			return true;
		}

		bool SetTag(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (InData.GetType() != model::JsonValue::Type::Int)
			{
				return false;
			}
			InNode->setTag(InData.AsInt());
			return true;
		}

		bool SetColor(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!InData.IsArray() || InData.GetArray().Size() < 3)
			{
				return false;
			}
			InNode->setColor(DataConverter::ValueToColor3B(InData));
			return true;
		}

		bool SetRotation(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!IsNumber(InData))
			{
				return false;
			}
			InNode->setRotation(InData.AsFloat());
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
			for (const std::string& Side : TrackSides)
			{
				InHull->removeChildByName(TrackNodeName(Side));
				cocos2d::SpriteFrame* const FirstFrame = FrameCache->getSpriteFrameByName(InPath + "_" + TrackNodeName(Side) + "_1");
				if (!FirstFrame)
				{
					continue;
				}
				cocos2d::Sprite* const Track = cocos2d::Sprite::createWithSpriteFrame(FirstFrame);
				Track->setName(TrackNodeName(Side));
				Track->setAnchorPoint(cocos2d::Vec2::ZERO);
				InHull->addChild(Track, TrackZ);
			}
		}

		cocos2d::Action* CreatePausedLoop(const TArray<cocos2d::SpriteFrame*>& InFrames, const float InDelay)
		{
			cocos2d::Animation* const CurrentAnimation = cocos2d::Animation::create();
			for (cocos2d::SpriteFrame* const Frame : InFrames)
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
			if (PatternAt == std::string::npos)
			{
				return false;
			}
			cocos2d::SpriteFrameCache* const FrameCache = cocos2d::SpriteFrameCache::getInstance();
			for (const std::string& Side : TrackSides)
			{
				cocos2d::Node* const Track = InHull->getChildByName(TrackNodeName(Side));
				if (!Track)
				{
					continue;
				}
				std::string Pattern = InMovePattern;
				Pattern.replace(PatternAt, MoveFramePattern.size(), TrackNodeName(Side) + "_%d");
				TArray<cocos2d::SpriteFrame*> Frames;
				while (cocos2d::SpriteFrame* const Frame = FrameCache->getSpriteFrameByName(cocos2d::StringUtils::format(Pattern.c_str(), static_cast<int>(Frames.Size()) + 1)))
				{
					Frames.PushBack(Frame);
				}
				if (!Frames.IsEmpty())
				{
					Track->runAction(CreatePausedLoop(Frames, InDelay));
				}
			}
			return true;
		}

		bool SetTexture(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!InData.IsString())
			{
				return false;
			}

			const std::string Path = InData.AsString();
			cocos2d::SpriteFrame* CurrentSpriteFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Path);
			if (!CurrentSpriteFrame && !cocos2d::FileUtils::getInstance()->isFileExist(Path))
			{
				CurrentSpriteFrame = GameContext::Get<ResourceManager>().FindFrame(Path);
			}

			switch (InNodeType)
			{
				case ENodeType::ImageView:
				{
					auto CurrentImageView = static_cast<cocos2d::ui::ImageView*>(InNode);
					if (CurrentSpriteFrame)
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
					if (HullFrame && TurretFrame)
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
					else if (CurrentSpriteFrame)
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
		bool SetMoveAnimation(const model::JsonValue& InData, cocos2d::Node* const InNode, [[maybe_unused]] const ENodeType InNodeType)
		{
			if (!InData.IsObject())
			{
				return false;
			}

			const model::JsonValue* const Name = InData.GetMembers().Find("name");
			const model::JsonValue* const FrameCount = InData.GetMembers().Find("frames_count");
			if (!Name || !FrameCount)
			{
				return false;
			}

			const model::JsonValue* const Delay = InData.GetMembers().Find("delay");
			const float CurrentDelay = Delay ? Delay->AsFloat() : DefaultAnimationFrameDelay;

			const std::string Pattern = Name->AsString();
			const int CurrentCount = FrameCount->AsInt();
			if (HasTrackLayers(InNode))
			{
				return SetTrackAnimations(InNode, Pattern, CurrentDelay);
			}

			ResourceManager& Resources = GameContext::Get<ResourceManager>();
			TArray<cocos2d::SpriteFrame*> Frames;
			for (int Index = 1; Index <= CurrentCount; ++Index)
			{
				if (cocos2d::SpriteFrame* const CurrentFrame = Resources.FindFrame(cocos2d::StringUtils::format(Pattern.c_str(), Index)))
				{
					Frames.PushBack(CurrentFrame);
				}
			}
			if (Frames.IsEmpty())
			{
				return false;
			}
			InNode->runAction(CreatePausedLoop(Frames, CurrentDelay));
			return true;
		}

		const TArray<std::pair<const char*, ParamSetter>>& ParamSetters()
		{
			static const TArray<std::pair<const char*, ParamSetter>> SETTERS = {
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
	void ObjectsFactory::ResolveMoveAnimation(model::JsonValue::Object& InOutValues) const
	{
		constexpr std::string_view VfxPrefix = "vfx.";
		const model::JsonValue* const AnimationReference = InOutValues.Find("move_animation");
		if (!AnimationReference || !AnimationReference->IsString())
		{
			return;
		}
		const std::string Reference = AnimationReference->AsString();
		const model::JsonValue& Effects = GameContext::Get<ConfigReader>().GetObjectData(config_refs::ResolveId(ids::configs::Effects)).Values;
		const model::JsonValue* const Effect = Reference.compare(0, VfxPrefix.size(), VfxPrefix) == 0
			? Effects.GetMembers().Find(Reference.substr(VfxPrefix.size()))
			: nullptr;
		if (!Effect || !Effect->IsObject())
		{
			CCLOGERROR("ObjectsFactory: unknown move_animation '%s'", Reference.c_str());
			InOutValues.Remove(FPersistentString::Find("move_animation"));
			return;
		}
		model::JsonValue::Object Animation;
		Animation.InsertOrAssign("name", model::JsonValue(Effect->GetString("frames", "")));
		Animation.InsertOrAssign("frames_count", model::JsonValue(Effect->GetInt("count", 0)));
		Animation.InsertOrAssign("delay", model::JsonValue(static_cast<double>(Effect->GetFloat("delay", DefaultAnimationFrameDelay))));
		InOutValues.InsertOrAssign("move_animation", model::JsonValue(std::move(Animation)));
	}

	model::JsonValue::Object ObjectsFactory::ReadValues(const std::string_view InConfigId) const
	{
		const ConfigReader& Reader = GameContext::Get<ConfigReader>();
		const model::JsonValue& Config = Reader.GetObjectData(InConfigId).Values;
		model::JsonValue::Object Result = Config.IsObject() ? Config.GetMembers() : model::JsonValue::Object();
		for (const char* const PartKey : { "look", "balance" })
		{
			const model::JsonValue* const Part = Result.Find(PartKey);
			if (!Part || !Part->IsString())
			{
				continue;
			}
			const FPersistentString PartPath = config_refs::ResolveName(Part->AsString());
			for (const auto& Entry : Reader.GetObjectData(PartPath).Values.GetMembers())
			{
				if (!Result.Find(Entry.Key.View()))
				{
					Result.InsertOrAssign(Entry.Key, Entry.Value);
				}
			}
		}
		ResolveMoveAnimation(Result);
		return Result;
	}

	FGameObjectHandle ObjectsFactory::CreateFromConfig(const std::string_view InConfigId, cocos2d::Node* const InParent, const model::JsonValue::Object& InOverrides)
	{
		model::JsonValue::Object ConfigData = ReadValues(InConfigId);
		if (ConfigData.IsEmpty())
		{
			return nullptr;
		}

		for (const auto& Override : InOverrides)
		{
			ConfigData.InsertOrAssign(Override.Key, Override.Value);
		}

		const model::JsonValue* const Type = ConfigData.Find("object_type");
		if (!Type)
		{
			CCLOGERROR("ObjectsFactory: no object_type in %.*s", static_cast<int>(InConfigId.size()), InConfigId.data());
			return nullptr;
		}
		FGameObjectHandle Result = Create(ParseObjectType(*Type), ConfigData, InParent);
		if (Tank* const CreatedTank = ObjectCast<Tank>(Result.Get()))
		{
			CreatedTank->ApplyDef(TankLoader::ReadTank(GameContext::Get<ConfigReader>().GetConfig(InConfigId)));
		}
		return Result;
	}

	FGameObjectHandle ObjectsFactory::Create(const EObjectType InType, const model::JsonValue::Object& InData, cocos2d::Node* const InParent)
	{
		if (!InParent)
		{
			return nullptr;
		}

		const model::JsonValue* const NodeType = InData.Find("node_type");
		if (!NodeType)
		{
			return nullptr;
		}

		cocos2d::Node* CurrentVisualNode = CreateNode(ParseNodeType(*NodeType), InData, InParent);
		if (!CurrentVisualNode)
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

		if (const model::JsonValue* const Name = InData.Find("name"))
		{
			Object->SetName(Name->AsString());
		}
		Object->SetOnDestroyCallback([this](GameObject* const InObject) { OnObjectDestroy(InObject); });

		Objects[InType].EmplaceBack(Object);
		return Object;
	}

	FGameObjectHandle ObjectsFactory::CreateVehicle(const model::JsonValue::Object& InData, cocos2d::Node* const InNode)
	{
		const model::JsonValue* const Type = InData.Find("type");
		if (!Type)
		{
			return nullptr;
		}

		switch (ParseVehicleType(*Type))
		{
			case EVehicleType::Tank:
			{
				return Tank::Create(InNode);
			}
			default:
				return nullptr;
		}
	}

	cocos2d::Node* ObjectsFactory::CreateNode(const ENodeType InType, const model::JsonValue::Object& InData, cocos2d::Node* const InParent)
	{
		cocos2d::Node* Result = nullptr;

		switch (InType)
		{
			case ENodeType::Sprite:
			{
				Result = cocos2d::Sprite::create();
				break;
			}
			default: // TODO: node, image view, button, label
				break;
		}

		if (Result && InParent)
		{
			InParent->addChild(Result);
			SetObjectParams(InData, Result, InType);
		}
		return Result;
	}

	FGameObjectHandle ObjectsFactory::FindObject(std::string_view InName, const EObjectType InType) const
	{
		for (const auto& TypeIt : Objects)
		{
			if (InType != EObjectType::Undefined && TypeIt.Key != InType)
			{
				continue;
			}
			for (const auto& Object : TypeIt.Value)
			{
				if (Object->GetName() == InName)
				{
					return Object;
				}
			}
		}
		return nullptr;
	}

	void ObjectsFactory::ForEachObject(const EObjectType InType, const TFunctionRef<void(const FGameObjectHandle&)> InAction) const
	{
		for (const auto& TypeIt : Objects)
		{
			if (InType != EObjectType::Undefined && TypeIt.Key != InType)
			{
				continue;
			}
			for (const auto& Object : TypeIt.Value)
			{
				InAction(Object);
			}
		}
	}

	FGameObjectHandle ObjectsFactory::FindObjectIf(const TFunctionRef<bool(const FGameObjectHandle&)> InPredicate) const
	{
		for (const auto& TypeIt : Objects)
		{
			for (const auto& Object : TypeIt.Value)
			{
				if (!Object->IsDestroying() && InPredicate(Object))
				{
					return Object;
				}
			}
		}
		return nullptr;
	}

	void ObjectsFactory::SetObjectParams(const model::JsonValue::Object& InData, cocos2d::Node* const InNode, const ENodeType InNodeType)
	{
		for (const auto& Setter : ParamSetters())
		{
			if (const model::JsonValue* const Value = InData.Find(Setter.first))
			{
				Setter.second(*Value, InNode, InNodeType);
			}
		}
	}

	void ObjectsFactory::OnObjectDestroy(GameObject* const InObject)
	{
		TArray<FGameObjectHandle>& CurrentObjects = Objects[InObject->GetType()];
		CurrentObjects.RemoveIf([InObject](const FGameObjectHandle& InItem) { return InItem.Get() == InObject; });
	}

} // namespace game

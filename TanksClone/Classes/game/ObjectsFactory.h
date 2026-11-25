#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "core/ObjectCast.h"
#include "game/objects/GameObject.h"
#include "templates/TFunctionRef.h"
#include "templates/TContainers.h"
#include "templates/TTemplateHelper.h"

namespace game
{
/**
 * Creates game objects (and their visual nodes) from ValueMap descriptions,
 * owns them, and destroys each one when its node leaves the scene.
 */
class ObjectsFactory : public FNonCopyableNonMovable
{
public:
	ObjectsFactory() = default;
	~ObjectsFactory() = default;

	FGameObjectHandle CreateFromConfig(std::string_view InConfigId, cocos2d::Node* InParent, const cocos2d::ValueMap& InOverrides = cocos2d::ValueMap());

	cocos2d::ValueMap ReadValues(std::string_view InConfigId) const;

	FGameObjectHandle Create(EObjectType InType, const cocos2d::ValueMap& InData, cocos2d::Node* InParent);
	cocos2d::Node* CreateNode(ENodeType InType, const cocos2d::ValueMap& InData, cocos2d::Node* InParent);

	FGameObjectHandle FindObject(std::string_view InName, EObjectType InType = EObjectType::Undefined) const;
	void ForEachObject(EObjectType InType, TFunctionRef<void(const FGameObjectHandle&)> InAction) const;
	FGameObjectHandle FindObjectIf(TFunctionRef<bool(const FGameObjectHandle&)> InPredicate) const;

	void SetObjectParams(const cocos2d::ValueMap& InData, cocos2d::Node* InNode, ENodeType InNodeType);

	template <typename TObject>
	THandle<TObject> CreateFromConfig(const std::string_view InConfigId, cocos2d::Node* const InParent, const cocos2d::ValueMap& InOverrides = cocos2d::ValueMap())
	{
		return ObjectCast<TObject>(CreateFromConfig(InConfigId, InParent, InOverrides));
	}

	template <typename TObject>
	THandle<TObject> FindObject(std::string_view InName) const
	{
		FGameObjectHandle Object = FindObject(InName, ObjectClass<TObject>::GetType());
		if (!Object)
		{
			return nullptr;
		}
		auto Result = ObjectCast<TObject>(Object);
		if (!Result)
		{
			return nullptr;
		}
		return Result;
	}

	template <typename TObjectType, typename TAction>
	void ForEachObject(TAction&& InAction) const
	{
		ForEachObject(ObjectClass<TObjectType>::GetType(), [&InAction](const FGameObjectHandle& InObject)
		{
			if(InObject)
			{
				InAction(*ObjectCast<TObjectType>(InObject.Get()));
			}
		});
	}

	template <typename TObjectType>
	THandle<TObjectType> FindObjectIf(TFunctionRef<bool(TObjectType&)> InPredicate) const
	{
		for ( const auto& TypeIt : Objects )
		{
			if ( !ObjectClass<TObjectType>::Matches(TypeIt.Key) )
			{
				continue;
			}
			for ( const auto& Object : TypeIt.Value )
			{
				if ( THandle<TObjectType> Typed = ObjectCast<TObjectType>(Object) )
				{
					if ( !Typed->IsDestroying() && InPredicate(*Typed) )
					{
						return Typed;
					}
				}
			}
		}
		return nullptr;
	}
	
private:
	void ResolveMoveAnimation(cocos2d::ValueMap& InOutValues) const;

	FGameObjectHandle CreateVehicle(const cocos2d::ValueMap& InData, cocos2d::Node* InNode);
	void OnObjectDestroy(GameObject* InObject);

	TMap<EObjectType, TArray<FGameObjectHandle>> Objects;
};
} // namespace game

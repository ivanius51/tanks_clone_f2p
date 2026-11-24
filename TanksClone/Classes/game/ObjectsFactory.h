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

namespace tanks
{
/**
 * Creates game objects (and their visual nodes) from ValueMap descriptions,
 * owns them, and destroys each one when its node leaves the scene.
 */
class ObjectsFactory
{
public:
	ObjectsFactory() = default;
	~ObjectsFactory() = default;
	ObjectsFactory(const ObjectsFactory&) = delete;
	ObjectsFactory& operator=(const ObjectsFactory&) = delete;

	GameObject* CreateFromConfig(const std::string& InConfigId, cocos2d::Node* InParent, const cocos2d::ValueMap& InOverrides = cocos2d::ValueMap());

	cocos2d::ValueMap ReadValues(const std::string& InConfigId) const;

	GameObject* Create(ObjectType InType, const cocos2d::ValueMap& InData, cocos2d::Node* InParent);
	cocos2d::Node* CreateNode(NodeType InType, const cocos2d::ValueMap& InData, cocos2d::Node* InParent);

	GameObject* FindObject(std::string_view InName, ObjectType InType = ObjectType::Undefined) const;
	void ForEachObject(ObjectType InType, TFunctionRef<void(GameObject&)> InAction) const;
	GameObject* FindObjectIf(TFunctionRef<bool(GameObject&)> InPredicate) const;

	void SetObjectParams(const cocos2d::ValueMap& InData, cocos2d::Node* InNode, NodeType InNodeType);

	template <typename TObject>
	TObject* CreateFromConfig(const std::string& InConfigId, cocos2d::Node* const InParent, const cocos2d::ValueMap& InOverrides = cocos2d::ValueMap())
	{
		return ObjectCast<TObject>(CreateFromConfig(InConfigId, InParent, InOverrides));
	}

	template <typename TObject>
	TObject* FindObject(std::string_view InName) const
	{
		return ObjectCast<TObject>(FindObject(InName, ObjectClass<TObject>::GetType()));
	}

	template <typename TObject, typename TAction>
	void ForEachObject(TAction&& InAction) const
	{
		ForEachObject(ObjectClass<TObject>::GetType(), [&InAction](GameObject& InObject)
		{
			if ( TObject* Typed = ObjectCast<TObject>(&InObject) )
			{
				InAction(*Typed);
			}
		});
	}

private:
	void ResolveMoveAnimation(cocos2d::ValueMap& InOutValues) const;

	std::unique_ptr<GameObject> CreateVehicle(const cocos2d::ValueMap& InData, cocos2d::Node* InNode);
	void OnObjectDestroy(GameObject* InObject);

	std::map<ObjectType, std::vector<std::unique_ptr<GameObject>>> Objects;
};
} // namespace tanks

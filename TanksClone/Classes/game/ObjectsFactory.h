#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "core/ObjectCast.h"
#include "game/objects/GameObject.h"

namespace tanks
{

// Creates game objects (and their visual nodes) from ValueMap descriptions,
// owns them, and destroys each one when its node leaves the scene.
class ObjectsFactory
{
public:
	static ObjectsFactory& GetInstance();

	// Creates an object from a config file, e.g. "configs/player_green_tank.json".
	// Keys in InOverrides replace the ones from the config.
	// Returns nullptr on failure. The factory keeps ownership of the result.
	GameObject* CreateFromConfig(const std::string& InConfigId, cocos2d::Node* InParent, const cocos2d::ValueMap& InOverrides = cocos2d::ValueMap());

	GameObject* Create(ObjectType InType, const cocos2d::ValueMap& InData, cocos2d::Node* InParent);
	cocos2d::Node* CreateNode(NodeType InType, const cocos2d::ValueMap& InData, cocos2d::Node* InParent);

	// With InType == ObjectType::Undefined all types are searched.
	GameObject* FindObject(std::string_view InName, ObjectType InType = ObjectType::Undefined) const;
	void ForEachObject(ObjectType InType, const std::function<void(GameObject&)>& InAction) const;
	// First live object (any type) for which InPredicate returns true, or nullptr.
	GameObject* FindObjectIf(const std::function<bool(GameObject&)>& InPredicate) const;

	// InNodeType is the type CreateNode made InNode as; setters static_cast by it.
	void SetObjectParams(const cocos2d::ValueMap& InData, cocos2d::Node* InNode, NodeType InNodeType);

	// TObject needs an ObjectClass specialization (core/ObjectCast.h) and must be complete at the call.
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
	ObjectsFactory() = default;
	~ObjectsFactory() = default;
	ObjectsFactory(const ObjectsFactory&) = delete;
	ObjectsFactory& operator=(const ObjectsFactory&) = delete;

	std::unique_ptr<GameObject> CreateVehicle(const cocos2d::ValueMap& InData, cocos2d::Node* InNode);
	void OnObjectDestroy(GameObject* InObject);

	std::map<ObjectType, std::vector<std::unique_ptr<GameObject>>> Objects;
};

} // namespace tanks

#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "cocos2d.h"

#include "core/ObjectEnums.h"
#include "game/GameCommand.h"

namespace tanks
{

// Base class of every game entity. Holds the gameplay data, while the visual
// representation is a cocos2d Node owned by the scene graph.
class GameObject
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	// Reads the gameplay parameters of the object description ("health", "damage",
	// "destroyable", "team"). Subclasses extend it with their own keys.
	virtual void Configure(const cocos2d::ValueMap& InData);

	virtual int GetHealth() const;
	int GetMaxHealth() const;
	virtual int GetDamage() const;

	virtual bool IsMovable() const;
	virtual bool IsDestroyable() const;
	virtual bool IsPassable() const;
	virtual bool IsFlyable() const;

	bool IsPlayer() const;
	void SetPlayer(bool bInIsPlayer);

	const std::string& GetName() const;
	void SetName(std::string_view InName);

	ObjectType GetType() const;
	Team GetTeam() const;
	void SetTeam(Team InTeam);
	cocos2d::Node* GetVisualNode() const;

	// Called when the object is deleted (its node left the scene).
	void SetOnDestroyCallback(const std::function<void(GameObject*)>& InOnDestroyCallback);
	// Called when the health reaches zero, right before the object is destroyed.
	void SetOnKilledCallback(const std::function<void(GameObject&)>& InOnKilledCallback);

	// Reduces the health. Indestructible objects ignore it.
	virtual void ApplyDamage(int InDamage, Team InSource);
	void Heal(int InAmount);
	// Kept between 1 and the maximum.
	void SetHealth(int InHealth);

	// Removes the object from the game. Deferred to the end of the frame, so it is
	// safe to call from anywhere (even from the object's own update).
	void Destroy();
	bool IsDestroying() const;

	virtual void OnCommandReceived(const GameCommand& InCommand);

	// Changes the maximum without refilling: a bigger maximum adds the difference to the health.
	void RescaleMaxHealth(int InMaxHealth);

protected:
	void SetMaxHealth(int InHealth);
	void SetDamage(int InDamage);

	void SetMovable(bool bInIsMovable);
	void SetDestroyable(bool bInIsDestroyable);
	void SetPassable(bool bInIsPassable);
	void SetFlyable(bool bInIsFlyable);
	void SetType(ObjectType InType);

	void SetVisualNode(cocos2d::Node* InVisualNode);

	// Starts calling Update() every frame, until the object is destroyed.
	void StartUpdating();
	virtual void Update(float InDelta);

	virtual void OnDamaged(int InDamage);
	virtual void OnKilled();
	// OnKilled, the killed callback, then Destroy.
	void Kill();
	virtual void OnDestroy();
	virtual bool Build() = 0;

private:
	int Health = 1;
	int MaxHealth = 1;
	int Damage = 0;

	bool bMovable = false;
	bool bDestroyable = true;
	bool bPassable = true;
	bool bFlyable = true;
	bool bIsPlayer = false;
	bool bDestroying = false;

	ObjectType ObjectKind = ObjectType::Undefined;
	Team OwnerTeam = Team::Neutral;
	std::string Name;
	cocos2d::Node* VisualNode = nullptr; // not owned: the parent node keeps it alive

	std::function<void(GameObject*)> OnDestroyCallback;
	std::function<void(GameObject&)> OnKilledCallback;
};

} // namespace tanks

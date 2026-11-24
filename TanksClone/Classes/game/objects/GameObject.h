#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "cocos2d.h"

#include "core/ObjectEnums.h"
#include "game/GameCommand.h"

namespace tanks
{
/**
 * Base class of every game entity. Holds the gameplay data, while the visual
 * representation is a cocos2d Node owned by the scene graph.
 */
class GameObject
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	virtual void Configure(const cocos2d::ValueMap& InData);

	virtual int GetHealth() const;
	int GetMaxHealth() const;
	virtual int GetDamage() const;

	virtual bool IsMovable() const;
	virtual bool IsDestroyable() const;
	const std::string& GetHitEffect() const;
	virtual bool IsPassable() const;
	virtual bool IsFlyable() const;

	bool IsPlayer() const;
	void SetPlayer(bool bInIsPlayer);

	const std::string& GetName() const;
	void SetName(std::string_view InName);

	ObjectType GetType() const;
	Team GetTeam() const;
	Team GetLastDamageSource() const;
	int GetLastOverkill() const;
	const std::string& GetTexturePath() const;
	void SetTeam(Team InTeam);
	cocos2d::Node* GetVisualNode() const;

	void SetOnDestroyCallback(const std::function<void(GameObject*)>& InOnDestroyCallback);
	void SetOnKilledCallback(const std::function<void(GameObject&)>& InOnKilledCallback);

	virtual void ApplyDamage(int InDamage, Team InSource);
	void Heal(int InAmount);
	void SetHealth(int InHealth);

	void Destroy();
	bool IsDestroying() const;

	virtual void OnCommandReceived(const GameCommand& InCommand);

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

	void StartUpdating();
	virtual void Update(float InDelta);

	virtual void OnDamaged(int InDamage);
	virtual void OnKilled();
	void Kill();
	virtual void OnDestroy();
	virtual bool Build() = 0;

private:
	int Health = 1;
	int MaxHealth = 1;
	int Damage = 0;

	bool bMovable = false;
	bool bDestroyable = true;
	std::string HitEffect;
	bool bPassable = true;
	bool bFlyable = true;
	bool bIsPlayer = false;
	bool bDestroying = false;
	Team LastDamageSource = Team::Neutral;
	int LastOverkill = 0;
	std::string TexturePath;

	ObjectType ObjectKind = ObjectType::Undefined;
	Team OwnerTeam = Team::Neutral;
	std::string Name;
	cocos2d::Node* VisualNode = nullptr;

	std::function<void(GameObject*)> OnDestroyCallback;
	std::function<void(GameObject&)> OnKilledCallback;
};
} // namespace tanks

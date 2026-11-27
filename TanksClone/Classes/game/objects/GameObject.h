#pragma once

#include <string>
#include <string_view>

#include "cocos2d.h"

#include "core/ObjectEnums.h"
#include "game/GameCommand.h"
#include "model/JsonValue.h"
#include "templates/TFunction.h"
#include "templates/TStoredString.h"
#include "templates/TRefCountedHandle.h"

namespace game
{
	class GameObject;
	using FGameObjectHandle = THandle<GameObject>;
/**
 * Base class of every game entity. Holds the gameplay data, while the visual
 * representation is a cocos2d Node owned by the scene graph.
 */
class GameObject : public TRefCounted
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;
	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	virtual void Configure(const model::JsonValue::Object& InData);

	virtual int GetHealth() const;
	int GetMaxHealth() const;
	virtual int GetDamage() const;

	virtual bool IsMovable() const;
	virtual bool IsDestroyable() const;
	FPersistentString GetHitEffect() const;
	virtual bool IsPassable() const;
	virtual bool IsFlyable() const;

	bool IsPlayer() const;
	void SetPlayer(bool bInIsPlayer);

	FTransientString GetName() const;
	void SetName(std::string_view InName);

	EObjectType GetType() const;
	ETeam GetTeam() const;
	ETeam GetLastDamageSource() const;
	int GetLastOverkill() const;
	FPersistentString GetTexturePath() const;
	void SetTeam(ETeam InTeam);
	cocos2d::Node* GetVisualNode() const;

	void SetOnDestroyCallback(const TFunction<void(GameObject*)>& InOnDestroyCallback);
	void SetOnKilledCallback(const TFunction<void(GameObject&)>& InOnKilledCallback);

	virtual void ApplyDamage(int InDamage, ETeam InSource);
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
	void SetType(EObjectType InType);

	void SetVisualNode(cocos2d::Node* InVisualNode);

	void StartUpdating();
	virtual void Update(float InDelta);

	virtual void OnDamaged(int InDamage);
	virtual void OnKilled();
	virtual void OnDestroying();
	void Kill();
	virtual void OnDestroy();
	virtual bool Build() = 0;

private:
	int Health = 1;
	int MaxHealth = 1;
	int Damage = 0;

	bool bMovable = false;
	bool bDestroyable = true;
	FPersistentString HitEffect;
	bool bPassable = true;
	bool bFlyable = true;
	bool bIsPlayer = false;
	bool bDestroying = false;
	ETeam LastDamageSource = ETeam::Neutral;
	int LastOverkill = 0;
	FPersistentString TexturePath;

	EObjectType ObjectKind = EObjectType::Undefined;
	ETeam OwnerTeam = ETeam::Neutral;
	FTransientString Name;
	cocos2d::Node* VisualNode = nullptr;

	TFunction<void(GameObject*)> OnDestroyCallback;
	TFunction<void(GameObject&)> OnKilledCallback;
};
} // namespace game

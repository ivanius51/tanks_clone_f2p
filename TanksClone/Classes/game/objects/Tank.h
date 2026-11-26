#pragma once

#include <memory>
#include <string_view>

#include "game/TankLook.h"
#include "game/ai/TankAi.h"
#include "game/components/Ability.h"
#include "game/components/Buff.h"
#include "game/components/Stats.h"
#include "game/components/Weapon.h"
#include "game/objects/MovableObject.h"
#include "game/objects/TankDef.h"
#include "model/DamageState.h"
#include "model/DifficultyBook.h"
#include "model/PhaseTracker.h"
#include "templates/TFunction.h"
#include "templates/TUniquePtr.h"
#include "templates/TContainers.h"

namespace game
{

class Tank;
using FTankHandle = THandle<Tank>;
/**
 * A tank, either the player's or an enemy's. What it can do comes from data:
 * - stats (health, damage, armor, speed, reload, ammo) are base values from the config,
 * changed by buffs (permanent ones from upgrades, timed ones from abilities and bonuses);
 * - abilities (Shoot, shield...) are listed in the config;
 * - "ai" makes it drive itself.
 */
class Tank : public MovableObject
{
public:
	static FTankHandle Create(cocos2d::Node* InNode);

	~Tank() override;

	void ApplyDef(const TankDef& InDef);

	const StatSet& GetStats() const;
	BuffSet& GetBuffs();
	const BuffSet& GetBuffs() const;
	void RefreshStats();
	bool IsOnMarklessGround() const { return bOnMarklessGround; }
	bool IsConcealed() const { return ConcealSightTiles > 0.0f && RevealTimer <= 0.0f; }
	float GetConcealSightTiles() const { return ConcealSightTiles; }
	void RestoreFull();

	size_t GetAbilityCount() const;
	const Ability* GetAbility(size_t InIndex) const;
	bool UseAbility(size_t InIndex);
	void SetFrozen(bool bInFrozen);
	bool IsImmobilized() const;
	void SetBulletClash(bool bInClash);
	void SetFriendlyFire(bool bInFriendlyFire);
	void SetAiSuspended(bool bInSuspended);
	bool IsAiSuspended() const;
	bool UseSecondaryAbility();
	bool Shoot();
	bool Fire();
	bool IsShotQueued() const;
	void SetTriggerHeld(bool bInHeld);
	bool IsTriggerHeld() const;
	bool EnableAutoFire(std::string_view InWeaponId);
	bool CanAutoFire() const;
	bool UnlockWeapon(std::string_view InWeaponId);
	bool ImproveWeapon(std::string_view InWeaponId, int InDamageBonus, float InReloadScale);
	int GetShotDamage(const WeaponDef& InWeapon) const;
	void ApplyDifficulty(const model::DifficultyDef& InMode);
	TankAi* GetAi() const;
	float GetBulletSpeedScale() const;

	size_t GetWeaponCount() const;
	const WeaponDef& GetActiveWeapon() const;
	size_t GetActiveWeaponIndex() const;
	bool SwitchWeapon();
	bool FireExtra(const WeaponDef& InWeapon);
	bool SelectWeapon(size_t InIndex);
	bool CanFireActiveWeapon() const;

	void AddAmmo(int InAmount);
	void SetAmmo(int InAmount);
	int GetAmmo() const;
	int GetMaxAmmo() const;
	bool HasUnlimitedAmmo() const;
	int GetArmor() const;
	float GetReloadProgress() const;
	float GetReloadRemaining() const;

	int GetDamage() const override;

	EEnemyType GetEnemyType() const;
	int GetScoreReward() const;
	int GetCoinReward() const;
	FPersistentString GetDropTable() const;
	FPersistentString GetSplitInto() const;
	int GetSplitCount() const;
	float GetSplitSpacingTiles() const;
	int GetPhase() const;
	bool HasPhases() const;
	void SetOnPhaseCallback(const TFunction<void(Tank&, const TankPhaseDef&)>& InCallback);

	void ApplyDamage(int InDamage, ETeam InSource) override;
	void ApplyDamageFrom(int InDamage, ETeam InSource, const cocos2d::Vec2& InShotDirection);
	bool HasFrontalShield() const;
	void OnCommandReceived(const GameCommand& InCommand) override;

protected:
	void Update(float InDelta) override;
	bool IsBlockedByGround(const cocos2d::Vec2& InPosition) const override;
	void OnBlocked() override;
	void OnTurnFinished() override;
	void PlayShieldHit();
	bool SpawnVolley(const WeaponDef& InWeapon);
	float FindFoeAhead() const;
	void UpdateBurst(float InDelta);
	void OnKilled() override;

private:
	TankLook& GetLook();
	Tank();

	bool Build() override;
	void UpdateShieldVisual();
	void UpdateBodyShadow();
	cocos2d::Vec2 BodyShadowOffset;
	cocos2d::Node* BodyShadowNode = nullptr;
	void UpdatePhases();
	void StartPhase(const TankPhaseDef& InPhase);
	void AddAbility(const AbilityDef& InDef);
	void AddPermanentBuff(std::string_view InId);
	float GetActiveReloadTime() const;

	StatSet Stats;
	BuffSet Buffs;
	TArray<TUniquePtr<Ability>> Abilities;
	TArray<WeaponDef> Weapons;
	model::ResourceId HitSound;
	model::ResourceId ArmorSound;
	model::ResourceId ExplodeSound;
	TArray<float> ReloadTimers;
	TArray<float> Heat;
	TArray<float> SinceShot;
	size_t ActiveWeapon = 0;
	bool bUnlimitedAmmo = false;
	bool bTriggerHeld = false;
	bool bShotQueued = false;
	TArray<WeaponDef> LockedWeapons;
	int BurstLeft = 0;
	float BurstTimer = 0.0f;
	size_t BurstWeapon = 0;

	int Ammo = 0;

	TUniquePtr<TankAi> Ai;
	TUniquePtr<TankLook> Look;
	bool bOnMarklessGround = false;
	float ConcealSightTiles = 0.0f;
	float ConcealOpacity = 1.0f;
	float ConcealRevealTime = 0.0f;
	float RevealTimer = 0.0f;
	bool bConcealApplied = false;
	void UpdateConcealment(float InDelta);
	void UpdateGround();
	model::DamageState SideHits;
	bool bFrozen = false;
	bool bAiSuspended = false;
	EEnemyType EnemyKind = EEnemyType::Medium;
	int ScoreReward = 100;
	int CoinReward = 10;
	float DamageScale = 1.0f;
	float ReloadDifficulty = 1.0f;
	float BulletSpeedScale = 1.0f;
	float GroundSpeedScale = 1.0f;
	float CooldownScale = 1.0f;
	bool bFriendlyFire = false;
	float FriendlyFireShare = 1.0f;
	bool bBulletClash = false;
	TArray<AbilityDef> AbilityDefs;
	FPersistentString DropTable;
	FPersistentString SplitInto;
	int SplitCount = 0;
	float SplitSpacingTiles = 0.0f;
	bool bFrontalShield = false;
	float RearDamageScale = 1.0f;
	TArray<TankPhaseDef> Phases;
	model::PhaseTracker PhaseState;
	TFunction<void(Tank&, const TankPhaseDef&)> OnPhaseCallback;
};
} // namespace game

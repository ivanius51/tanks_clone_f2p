#pragma once

#include <memory>
#include <string_view>
#include <vector>

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

namespace tanks
{
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
	static std::unique_ptr<Tank> Create(cocos2d::Node* InNode);

	~Tank() override;

	void ApplyDef(const TankDef& InDef);

	const StatSet& GetStats() const;
	BuffSet& GetBuffs();
	const BuffSet& GetBuffs() const;
	void RefreshStats();
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

	EnemyType GetEnemyType() const;
	int GetScoreReward() const;
	int GetCoinReward() const;
	const std::string& GetDropTable() const;
	const std::string& GetSplitInto() const;
	int GetSplitCount() const;
	float GetSplitSpacingTiles() const;
	int GetPhase() const;
	bool HasPhases() const;
	void SetOnPhaseCallback(const TFunction<void(Tank&, const TankPhaseDef&)>& InCallback);

	void ApplyDamage(int InDamage, Team InSource) override;
	void ApplyDamageFrom(int InDamage, Team InSource, const cocos2d::Vec2& InShotDirection);
	bool HasFrontalShield() const;
	void OnCommandReceived(const GameCommand& InCommand) override;

protected:
	void Update(float InDelta) override;
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
	void UpdatePhases();
	void StartPhase(const TankPhaseDef& InPhase);
	void AddAbility(const AbilityDef& InDef);
	void AddPermanentBuff(const std::string& InId);
	float GetActiveReloadTime() const;

	StatSet Stats;
	BuffSet Buffs;
	std::vector<std::unique_ptr<Ability>> Abilities;
	std::vector<WeaponDef> Weapons;
	model::ResourceId HitSound;
	model::ResourceId ArmorSound;
	model::ResourceId ExplodeSound;
	std::vector<float> ReloadTimers;
	std::vector<float> Heat;
	std::vector<float> SinceShot;
	size_t ActiveWeapon = 0;
	bool bUnlimitedAmmo = false;
	bool bTriggerHeld = false;
	bool bShotQueued = false;
	std::vector<WeaponDef> LockedWeapons;
	int BurstLeft = 0;
	float BurstTimer = 0.0f;
	size_t BurstWeapon = 0;

	int Ammo = 0;

	std::unique_ptr<TankAi> Ai;
	std::unique_ptr<TankLook> Look;
	model::DamageState SideHits;
	bool bFrozen = false;
	bool bAiSuspended = false;
	EnemyType EnemyKind = EnemyType::Medium;
	int ScoreReward = 100;
	int CoinReward = 10;
	float DamageScale = 1.0f;
	float ReloadDifficulty = 1.0f;
	float BulletSpeedScale = 1.0f;
	float CooldownScale = 1.0f;
	bool bFriendlyFire = false;
	float FriendlyFireShare = 1.0f;
	bool bBulletClash = false;
	std::vector<AbilityDef> AbilityDefs;
	std::string DropTable;
	std::string SplitInto;
	int SplitCount = 0;
	float SplitSpacingTiles = 0.0f;
	bool bFrontalShield = false;
	float RearDamageScale = 1.0f;
	std::vector<TankPhaseDef> Phases;
	model::PhaseTracker PhaseState;
	TFunction<void(Tank&, const TankPhaseDef&)> OnPhaseCallback;
};
} // namespace tanks

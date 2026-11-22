#pragma once

#include <functional>
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

namespace tanks
{

// A tank, either the player's or an enemy's. What it can do comes from data:
//  - stats (health, damage, armor, speed, reload, ammo) are base values from the config,
//    changed by buffs (permanent ones from upgrades, timed ones from abilities and bonuses);
//  - abilities (Shoot, shield...) are listed in the config;
//  - "ai" makes it drive itself.
class Tank : public MovableObject
{
public:
	// Returns nullptr if the tank could not be built.
	static std::unique_ptr<Tank> Create(cocos2d::Node* InNode);

	~Tank() override;

	// Called after Configure: base stats, weapons, abilities, permanent buffs, rewards and AI.
	void ApplyDef(const TankDef& InDef);

	const StatSet& GetStats() const;
	BuffSet& GetBuffs();
	const BuffSet& GetBuffs() const;
	// Recalculates the stats after the buffs changed.
	void RefreshStats();
	// Full health and full ammo.
	void RestoreFull();

	size_t GetAbilityCount() const;
	const Ability* GetAbility(size_t InIndex) const;
	bool UseAbility(size_t InIndex);
	// While suspended (a ram charging or stunned) the AI gives no commands.
	// A frozen tank keeps its look (lamp, smoke, damage) but runs no AI, abilities or shots: for debug galleries.
	void SetFrozen(bool bInFrozen);
	bool IsImmobilized() const;
	void SetBulletClash(bool bInClash);
	void SetFriendlyFire(bool bInFriendlyFire);
	void SetAiSuspended(bool bInSuspended);
	bool IsAiSuspended() const;
	// Uses the first ready ability after the primary one.
	bool UseSecondaryAbility();
	// The primary ability (normally firing).
	bool Shoot();
	// Fires the active weapon. Used by the Shoot ability: it fails without ammo or while reloading.
	// While turning it fails too, but the shot is queued and fired when the turn ends.
	bool Fire();
	bool IsShotQueued() const;
	// While the trigger is held the tank fires every time the active weapon is ready.
	void SetTriggerHeld(bool bInHeld);
	bool IsTriggerHeld() const;
	// The weapon with this id fires while the trigger is held from now on. False if the tank has no such weapon.
	bool EnableAutoFire(std::string_view InWeaponId);
	bool CanAutoFire() const;
	// Moves a "locked" weapon of the config into the usable ones. False if there is no such locked weapon.
	bool UnlockWeapon(std::string_view InWeaponId);
	// Upgrades one weapon, locked or not: more damage per bullet and a scaled reload.
	bool ImproveWeapon(std::string_view InWeaponId, int InDamageBonus, float InReloadScale);
	int GetShotDamage(const WeaponDef& InWeapon) const;
	// Scales the maximum health (refilling it) and every shot's damage, for the difficulty mode.
	void ApplyDifficulty(const model::DifficultyDef& InMode);
	TankAi* GetAi() const;
	float GetBulletSpeedScale() const;

	// The guns from "weapons"; only the active one fires. Every gun reloads on its own.
	size_t GetWeaponCount() const;
	const WeaponDef& GetActiveWeapon() const;
	size_t GetActiveWeaponIndex() const;
	// Makes the next gun active. Returns false if the tank has only one.
	bool SwitchWeapon();
	// Fires a weapon that is not in the weapons list (an ability's own), without ammo or reload.
	bool FireExtra(const WeaponDef& InWeapon);
	bool SelectWeapon(size_t InIndex);
	// True if the active gun has what it needs to fire (ammo), reloading aside.
	bool CanFireActiveWeapon() const;

	void AddAmmo(int InAmount);
	// Clamped to 0..max; does nothing with unlimited ammo.
	void SetAmmo(int InAmount);
	int GetAmmo() const;
	int GetMaxAmmo() const;
	bool HasUnlimitedAmmo() const;
	int GetArmor() const;
	// Of the active weapon: 0 = just fired, 1 = ready to fire.
	float GetReloadProgress() const;
	float GetReloadRemaining() const;

	int GetDamage() const override; // bullet damage

	// Enemy tanks only: the class from "enemy_type" and what killing it is worth ("score", "coins").
	EnemyType GetEnemyType() const;
	int GetScoreReward() const;
	int GetCoinReward() const;
	const std::string& GetDropTable() const;
	const std::string& GetSplitInto() const;
	int GetSplitCount() const;
	float GetSplitSpacingTiles() const;
	// Boss phases: -1 before the first one. The callback gets each phase as it starts (to call escorts).
	int GetPhase() const;
	void SetOnPhaseCallback(const std::function<void(Tank&, const TankPhaseDef&)>& InCallback);

	void ApplyDamage(int InDamage, Team InSource) override;
	// A shot flying in InShotDirection: a frontal shield stops it at the front, the back may take more.
	void ApplyDamageFrom(int InDamage, Team InSource, const cocos2d::Vec2& InShotDirection);
	bool HasFrontalShield() const;
	void OnCommandReceived(const GameCommand& InCommand) override;

protected:
	void Update(float InDelta) override;
	void OnBlocked() override;
	void OnTurnFinished() override;
	// A ripple on the shield bubble when it stops a hit.
	void PlayShieldHit();
	// Every pellet of one shot of the weapon. False if none could be created.
	bool SpawnVolley(const WeaponDef& InWeapon);
	// Distance along the facing to the nearest foe in line ahead; 0 if there is none.
	float FindFoeAhead() const;
	// The rest of a burst follows the first shot without reloading.
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
	std::vector<float> ReloadTimers; // one per weapon
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
	std::vector<AbilityDef> AbilityDefs; // one per ability, to configure them again
	std::string DropTable;
	std::string SplitInto;
	int SplitCount = 0;
	float SplitSpacingTiles = 0.0f;
	bool bFrontalShield = false;
	float RearDamageScale = 1.0f;
	std::vector<TankPhaseDef> Phases;
	model::PhaseTracker PhaseState;
	std::function<void(Tank&, const TankPhaseDef&)> OnPhaseCallback;
};

} // namespace tanks

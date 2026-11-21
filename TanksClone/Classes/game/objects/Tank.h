#pragma once

#include <memory>
#include <vector>

#include "game/ai/TankAi.h"
#include "game/components/Ability.h"
#include "game/components/Buff.h"
#include "game/components/Stats.h"
#include "game/components/Weapon.h"
#include "game/objects/MovableObject.h"
#include "game/objects/TankDef.h"

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

	// The guns from "weapons"; only the active one fires. Every gun reloads on its own.
	size_t GetWeaponCount() const;
	const WeaponDef& GetActiveWeapon() const;
	size_t GetActiveWeaponIndex() const;
	// Makes the next gun active. Returns false if the tank has only one.
	bool SwitchWeapon();
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

	void ApplyDamage(int InDamage, Team InSource) override;
	void OnCommandReceived(const GameCommand& InCommand) override;

protected:
	void Update(float InDelta) override;
	void OnBlocked() override;
	void OnTurnFinished() override;
	void OnKilled() override;

private:
	Tank();

	bool Build() override;
	void UpdateShieldVisual();
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

	int Ammo = 0;

	std::unique_ptr<TankAi> Ai;
	EnemyType EnemyKind = EnemyType::Medium;
	int ScoreReward = 100;
	int CoinReward = 10;
};

} // namespace tanks

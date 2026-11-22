#pragma once

#include <string>

#include "model/ResourceId.h"

namespace tanks
{

/** One gun of a tank, from the "weapons" list of its config. */
struct WeaponDef
{
	std::string Id;
	std::string NameKey;      // string id for the HUD
	std::string Icon;         // sprite frame of the HUD slot
	std::string BulletConfig; // config of the bullets it fires
	float ReloadTime = 0.0f;  // seconds; 0 = the tank's "ReloadTime" stat (upgrades and buffs apply)
	int Damage = 0;           // 0 = the tank's "Damage" stat
	int DamageBonus = 0;       // from this weapon's own upgrades
	float ReloadScale = 1.0f;  // from this weapon's own upgrades: below 1 reloads faster
	bool bUsesAmmo = true;     // false = fires without shells
	int Pellets = 1;           // bullets per shot, fanned out over Spread degrees
	float Spread = 0.0f;
	int BurstCount = 1;        // shots per trigger pull, BurstInterval seconds apart; ammo is used once
	float BurstInterval = 0.0f;
	bool bDropsBehind = false; // the bullet appears behind the tank (mines)
	bool bLobbed = false;      // the shell flies over everything and lands on the first foe ahead, within the lob range
	float LobMinTiles = 2.0f;
	float LobMaxTiles = 8.0f;
	bool bLocked = false;      // not usable until the unlock_weapon upgrade or a pickup unlocks it
	int HeatShots = 0;         // shots in a row before it overheats and waits CoolTime; 0 = never
	float CoolTime = 0.0f;     // seconds of the overheat wait; the heat also drains this fast while not firing
	float CoolDelay = 0.3f;    // seconds after a shot before the heat starts to drain
	bool bAutoFire = false;    // keeps firing while the trigger is held; others need the auto_fire_<id> upgrade
	model::ResourceId FireSound; // invalid = silent
};

} // namespace tanks

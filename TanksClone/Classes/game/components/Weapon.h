#pragma once

#include <string>

#include "model/MathTypes.h"

#include "model/ResourceId.h"

namespace game
{
/** One gun of a tank, from the "weapons" list of its config. */
struct WeaponDef
{
	std::string Id;
	std::string NameKey;
	std::string Icon;
	std::string BulletConfig;
	float ReloadTime = 0.0f;
	int Damage = 0;
	int DamageBonus = 0;
	float ReloadScale = 1.0f;
	bool bUsesAmmo = true;
	int Pellets = 1;
	float Spread = 0.0f;
	int BurstCount = 1;
	float BurstInterval = 0.0f;
	bool bDropsBehind = false;
	bool bLobbed = false;
	float LobMinTiles = 2.0f;
	float LobMaxTiles = 8.0f;
	bool bLocked = false;
	int HeatShots = 0;
	float CoolTime = 0.0f;
	float CoolDelay = 0.3f;
	bool bAutoFire = false;
	model::ResourceId FireSound;
	float FlashWidth = 1.0f;
	float FlashLength = 1.0f;
	float FlashSpeed = 1.0f;
	model::Color FlashTint;
};
} // namespace game

#pragma once

#include <map>
#include <string>
#include <vector>

#include "cocos2d.h"

#include "game/Effects.h"
#include "model/Cooldown.h"

namespace game
{
class Tank;

/** Smoke below a health share: how often a puff comes out and its colour (darker nearer death). */
/** A trail one vehicle leaves on the ground: segments of Frame laid every SpacingTiles, fading out after Seconds. */
struct GroundMarkSettings
{
	std::string Frame;
	float Spacing = 8.0f;
	float Seconds = 8.0f;
	unsigned char Opacity = 160;
	float WidthShare = 0.9f;
};

struct SmokeStage
{
	float Below = 0.3f;
	float Interval = 0.35f;
	cocos2d::Color3B Color = cocos2d::Color3B::GRAY;
	std::string Frame = "smoke_puff";
	float Scale = 1.0f;
};

/** How a destroyed tank goes: a flash, a burnt wreck (configs/objects/spawnables/props/tank_wreck.json: blocks, takes hits, then goes) that smokes, and pieces of the hull thrown out. */
struct WreckSettings
{
	cocos2d::Color3B FlashColor{ 255, 190, 110 };
	cocos2d::Color3B BurntColor{ 60, 52, 46 };
	float BurnSeconds = 0.35f;
	float SmokeSeconds = 4.0f;
	float SmokeInterval = 0.45f;
	cocos2d::Color3B SmokeColor{ 60, 58, 60 };
	std::string SmokeFrame = "smoke_puff";
	float SmokeScale = 1.0f;
	float SmokeTime = 0.9f;
	float SmokeRise = 14.0f;
	int PieceGrid = 3;
	int PieceCount = 4;
	float PieceDistance = 0.8f;
	float PieceSeconds = 0.6f;
	int CorpseHealth = 4;
	float OverkillShare = 0.5f;
	float TurretFlyTiles = 0.7f;
	float TurretFlySeconds = 0.7f;
	float TurretStaySeconds = 2.0f;
};

/** Numbers and colours of TankLook, from configs/views/tank_look.json. */
struct TankLookSettings
{
	cocos2d::Vec2 LampOffset{ 0.5f, 0.3f };
	float LampScale = 1.0f;
	float LampTintTime = 0.1f;
	bool bLampRandomSide = true;
	std::map<std::string, cocos2d::Color3B> LampColors;
	std::vector<float> CrackStages{ 0.75f, 0.5f, 0.25f };
	int MaxHoles = 6;
	float HoleMergeDistance = 0.15f;
	float HoleGrowth = 1.25f;
	float HoleMaxScale = 1.8f;
	float HoleSpread = 0.3f;
	float HoleEdge = 0.3f;
	float JitterRotation = 8.0f;
	float JitterScale = 0.08f;
	bool bJitterMirror = true;
	cocos2d::Vec2 SmokeOffset{ 0.5f, 0.12f };
	std::vector<SmokeStage> SmokeStages;
	float SmokeRise = 14.0f;
	float SmokeTime = 0.9f;
	std::string MuzzleEffect;
	float MuzzleBack = 0.0f;
	std::string FlareFrame = "lamp_glow";
	float FlareScale = 2.0f;
	float FlareSeconds = 0.08f;
	unsigned char FlareOpacity = 170;
	float ShotGlowSeconds = 0.12f;
	unsigned char ShotGlowOpacity = 0;
	cocos2d::Color3B FlareColor{ 255, 210, 140 };
	std::string DustEffect;
	float DustInterval = 0.25f;
	float DustOffset = 0.45f;
	GroundMarkSettings TrackMark;
	GroundMarkSettings WheelMark;
	std::string ImmobileEffect;
	float ImmobileInterval = 0.3f;
	WreckSettings Wreck;
	float MaskAlphaThreshold = 0.05f;

	static TankLookSettings Load();
};

/**
 * What a player can read off a tank without numbers: a glowing lamp whose colour tells the enemy's current behaviour,
 * cracks that grow in stages, bullet holes where shots land, smoke near death and dirt while something slows it down.
 */
class TankLook
{
public:
	explicit TankLook(const TankLookSettings& InSettings);
	void Update(Tank& InTank);
	void AddHit(Tank& InTank, const cocos2d::Vec2& InShotDirection);
	void OnShot(Tank& InTank, const cocos2d::Vec2& InMuzzle, const cocos2d::Vec2& InDirection, const effects::EffectStyle& InFlash) const;
	void PlayShotGlow(cocos2d::Node& InTankNode) const;
	void PlayDestroyed(Tank& InTank) const;

private:
	cocos2d::Node& GetDamageLayer(cocos2d::Node& InTankNode) const;
	cocos2d::Sprite* EnsureOverlay(cocos2d::Node& InTankNode, const std::string& InFrame, int InTag, int InZOrder) const;
	float GetOverlayScale(const cocos2d::Node& InTankNode) const;
	void Jitter(cocos2d::Sprite& InOverlay, float InBaseScale, bool bInFullTurn) const;
	void UpdateCracks(cocos2d::Node& InTankNode, float InHealthShare);
	void UpdateLamp(Tank& InTank, cocos2d::Node& InTankNode);
	void UpdateSmoke(cocos2d::Node& InTankNode, float InHealthShare);
	void UpdateDust(Tank& InTank, cocos2d::Node& InTankNode);
	void UpdateGroundMarks(Tank& InTank, cocos2d::Node& InTankNode);
	void UpdateImmobile(Tank& InTank, cocos2d::Node& InTankNode);
	void ClearHoles();
	void ThrowPieces(cocos2d::Sprite& InTankSprite) const;
	void ThrowTurret(cocos2d::Sprite& InTankSprite) const;
	int GetCorpseHealth(const Tank& InTank) const;

	TankLookSettings Settings;
	model::Cooldown SmokeTimer;
	model::Cooldown DustTimer;
	cocos2d::Vec2 LastMarkPosition;
	bool bHasMarkPosition = false;
	model::Cooldown ImmobileTimer;
	bool bImmobileLeft = false;
	int CrackStage = 0;
	cocos2d::Color3B LampColor = cocos2d::Color3B::WHITE;
	std::vector<cocos2d::Sprite*> Holes;
};
} // namespace game

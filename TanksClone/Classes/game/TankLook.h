#pragma once

#include <map>
#include <string>
#include <vector>

#include "cocos2d.h"

namespace tanks
{

class Tank;

/** Smoke below a health share: how often a puff comes out and its colour (darker nearer death). */
struct SmokeStage
{
	float Below = 0.3f;
	float Interval = 0.35f;
	cocos2d::Color3B Color = cocos2d::Color3B::GRAY;
	std::string Frame = "smoke_puff";
	float Scale = 1.0f;
};

/** How a destroyed tank goes: a flash, a burnt wreck (configs/tank_wreck.json: blocks, takes hits, then goes) that smokes, and pieces of the hull thrown out. */
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
	int PieceGrid = 3;              // the hull is cut into Grid x Grid pieces
	int PieceCount = 4;             // how many of them fly off
	float PieceDistance = 0.8f;     // tiles
	float PieceSeconds = 0.6f;
	int CorpseHealth = 4;           // hits of chip damage the corpse takes (a player shot does 2)
	float OverkillShare = 0.5f;     // overkill of this share of the tank's max health leaves a corpse with 1
	float TurretFlyTiles = 0.7f;
	float TurretFlySeconds = 0.7f;
	float TurretStaySeconds = 2.0f;
};

/** Numbers and colours of TankLook, from configs/tank_look.json. */
struct TankLookSettings
{
	cocos2d::Vec2 LampOffset{ 0.5f, 0.3f };      // share of the tank sprite size
	float LampScale = 1.0f;
	float LampTintTime = 0.1f;
	bool bLampRandomSide = true;                // back-left or back-right, picked per tank
	std::map<std::string, cocos2d::Color3B> LampColors; // AI tactic name -> colour
	std::vector<float> CrackStages{ 0.75f, 0.5f, 0.25f }; // health share for each next crack frame
	int MaxHoles = 6;
	float HoleMergeDistance = 0.15f;             // share of the tank size: a hit this close deepens an old hole
	float HoleGrowth = 1.25f;
	float HoleMaxScale = 1.8f;
	float HoleSpread = 0.3f;                     // scatter along the hit face, share of the tank size
	float HoleEdge = 0.3f;                       // distance of a hole from the centre towards the hit face, share of the tank size
	float JitterRotation = 8.0f;                 // degrees, either way: every overlay is placed a bit differently
	float JitterScale = 0.08f;
	bool bJitterMirror = true;
	cocos2d::Vec2 SmokeOffset{ 0.5f, 0.12f };    // the engine at the back, share of the tank sprite size
	std::vector<SmokeStage> SmokeStages;          // ordered by health share, highest first
	float SmokeRise = 14.0f;
	float SmokeTime = 0.9f;
	std::string MuzzleEffect;                    // ids in configs/effects.json; empty = none
	float MuzzleBack = 0.0f;                     // pixels the flash sits back from the barrel tip, over the tank
	std::string FlareFrame = "lamp_glow";        // a bright see-through blink over the barrel with every shot
	float FlareScale = 2.0f;
	float FlareSeconds = 0.08f;
	unsigned char FlareOpacity = 170;
	float ShotGlowSeconds = 0.12f;
	unsigned char ShotGlowOpacity = 0;
	cocos2d::Color3B FlareColor{ 255, 210, 140 };
	std::string DustEffect;
	float DustInterval = 0.25f;
	float DustOffset = 0.45f;                    // behind the centre, share of the tank sprite height
	std::string ImmobileEffect;                  // sparks from the tracks while side hits keep the tank stopped
	float ImmobileInterval = 0.3f;
	WreckSettings Wreck;
	float MaskAlphaThreshold = 0.05f;            // overlays are drawn only where the tank sprite is more opaque than this

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
	void Update(Tank& InTank, float InDelta);
	void AddHit(Tank& InTank, const cocos2d::Vec2& InShotDirection);
	void OnShot(Tank& InTank, const cocos2d::Vec2& InMuzzle, const cocos2d::Vec2& InDirection) const;
	void PlayShotGlow(cocos2d::Node& InTankNode) const;
	void PlayDestroyed(Tank& InTank) const;

private:
	cocos2d::Node& GetDamageLayer(cocos2d::Node& InTankNode) const;
	cocos2d::Sprite* EnsureOverlay(cocos2d::Node& InTankNode, const std::string& InFrame, int InTag, int InZOrder) const;
	float GetOverlayScale(const cocos2d::Node& InTankNode) const;
	void Jitter(cocos2d::Sprite& InOverlay, float InBaseScale, bool bInFullTurn) const;
	void UpdateCracks(cocos2d::Node& InTankNode, float InHealthShare);
	void UpdateLamp(Tank& InTank, cocos2d::Node& InTankNode);
	void UpdateSmoke(cocos2d::Node& InTankNode, float InHealthShare, float InDelta);
	void UpdateDust(Tank& InTank, cocos2d::Node& InTankNode, float InDelta);
	void UpdateImmobile(Tank& InTank, cocos2d::Node& InTankNode, float InDelta);
	void ClearHoles();
	void ThrowPieces(cocos2d::Sprite& InTankSprite) const;
	void ThrowTurret(cocos2d::Sprite& InTankSprite) const;
	int GetCorpseHealth(const Tank& InTank) const;

	TankLookSettings Settings;
	float SmokeTimer = 0.0f;
	float DustTimer = 0.0f;
	float ImmobileTimer = 0.0f;
	bool bImmobileLeft = false;
	int CrackStage = 0;
	cocos2d::Color3B LampColor = cocos2d::Color3B::WHITE;
	std::vector<cocos2d::Sprite*> Holes;
};

} // namespace tanks

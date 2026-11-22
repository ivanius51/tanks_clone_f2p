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
	std::string DustEffect;
	float DustInterval = 0.25f;
	float DustOffset = 0.45f;                    // behind the centre, share of the tank sprite height

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

private:
	cocos2d::Sprite* EnsureOverlay(cocos2d::Node& InTankNode, const std::string& InFrame, int InTag, int InZOrder) const;
	float GetOverlayScale(const cocos2d::Node& InTankNode) const;
	void Jitter(cocos2d::Sprite& InOverlay, float InBaseScale, bool bInFullTurn) const;
	void UpdateCracks(cocos2d::Node& InTankNode, float InHealthShare);
	void UpdateLamp(Tank& InTank, cocos2d::Node& InTankNode);
	void UpdateSmoke(cocos2d::Node& InTankNode, float InHealthShare, float InDelta);
	void UpdateDust(Tank& InTank, cocos2d::Node& InTankNode, float InDelta);
	void ClearHoles(cocos2d::Node& InTankNode);

	TankLookSettings Settings;
	float SmokeTimer = 0.0f;
	float DustTimer = 0.0f;
	int CrackStage = 0;
	cocos2d::Color3B LampColor = cocos2d::Color3B::WHITE;
	std::vector<cocos2d::Sprite*> Holes;
};

} // namespace tanks

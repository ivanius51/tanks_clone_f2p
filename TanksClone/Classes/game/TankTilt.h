#pragma once

#include <string>

#include "cocos2d.h"

namespace game
{

/** The tilted look of a tank, from "tilt" in configs/views/tank_look.json; every length is a share of the sprite height. */
struct TankTiltSettings
{
	bool bEnabled = true;
	float Squash = 0.78f;
	float WallShare = 0.07f;
	float TurretRaiseShare = 0.1f;
	float TurretShadowShare = 0.04f;
	unsigned char TurretShadowOpacity = 110;
	cocos2d::Color3B WallColor{ 30, 32, 30 };

	static TankTiltSettings Load();
};

/**
 * Makes a top-down tank look slightly tilted without new art: the whole tank is squashed along the screen's vertical axis
 * (whatever its heading), a dark wall shows under the hull and tracks, and the turret sits raised over a shadow.
 * Everything is derived from the sprite size, so it works for any texture set.
 */
class TankTilt
{
public:
	explicit TankTilt(const TankTiltSettings& InSettings);
	void Update(cocos2d::Sprite& InHull) const;
	/** Takes the tilt off a hull: no squash, no wall, no turret shadow. */
	void Clear(cocos2d::Sprite& InHull) const;

private:
	void UpdateWall(cocos2d::Sprite& InHull, const cocos2d::Vec2& InWallOffset) const;
	void UpdateTurret(cocos2d::Sprite& InHull, const cocos2d::Vec2& InRaise, float InHeight) const;
	cocos2d::Sprite& EnsureWallPiece(cocos2d::Sprite& InHull, const std::string& InName) const;

	TankTiltSettings Settings;
};
} // namespace game

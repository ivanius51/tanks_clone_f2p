#pragma once

#include <memory>
#include <vector>

#include "game/objects/PhysicsObject.h"
#include "model/WallMask.h"

namespace tanks
{

/**
 * A block of the map made of sub-cells (WallMask). Bullets chip cells away from the side they hit;
 * tanks and bullets collide only with the cells that still stand, so a chipped wall can be driven through.
 */
class Wall : public PhysicsObject
{
public:
	static std::unique_ptr<Wall> Create(cocos2d::Node* InNode);

	// Keys: everything of GameObject plus "cells" (sub-cells per side) and "cell_health" (hits of chip
	// damage a cell takes), "base". "destroyable": false walls are only chipped by bullets that break stone.
	void Configure(const cocos2d::ValueMap& InData) override;

	bool Blocks(const cocos2d::Rect& InRect) const override;
	bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const override;

	// Chips the cells under InArea (parent coordinates) from InSide. Returns how many cells were removed;
	// the wall is destroyed when none is left.
	int Chip(const cocos2d::Rect& InArea, model::HitSide InSide, int InDepth, int InDamage, bool bInBreaksStone);

	const model::WallMask& GetMask() const;
	// The base of the player ("base": true): the level is lost when it falls.
	bool IsBase() const;
	// A supply crate: the pickup config inside ("contents"), or a drops.json table to roll when it is empty.
	const std::string& GetContents() const;
	const std::string& GetDropTable() const;
	// A fuel barrel ("blast_radius_tiles", "blast_damage"): when it breaks, every tank in the radius is hurt.
	bool IsExplosive() const;

protected:
	void OnKilled() override;

private:
	Wall() = default;

	bool Build() override;
	float GetCellSize() const;
	void Redraw();
	void ThrowDebris(cocos2d::Sprite& InCell);

	model::WallMask Mask{ 4, 1 };
	bool bBase = false;
	std::string Contents;
	std::string DropTable;
	float BlastRadius = 0.0f;
	int BlastDamage = 0;
	std::vector<int> ShownHealth;  // cell health at the last redraw, to see which cells just broke
	float DebrisSeconds = 0.45f;
	float EdgeBleedOpacity = 0.0f;
	bool bBreakBlast = true;
	float DebrisDistance = 0.0f;
};

} // namespace tanks

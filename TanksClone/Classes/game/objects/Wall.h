#pragma once

#include <memory>
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
	// damage a cell takes). "destroyable": false walls are only chipped by bullets that break stone.
	void Configure(const cocos2d::ValueMap& InData) override;

	bool Blocks(const cocos2d::Rect& InRect) const override;
	bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const override;

	// Chips the cells under InArea (parent coordinates) from InSide. Returns how many cells were removed;
	// the wall is destroyed when none is left.
	int Chip(const cocos2d::Rect& InArea, model::HitSide InSide, int InDepth, int InDamage, bool bInBreaksStone);

	const model::WallMask& GetMask() const;

protected:
	void OnKilled() override;

private:
	Wall() = default;

	bool Build() override;
	float GetCellSize() const;
	void Redraw();

	model::WallMask Mask{ 4, 1 };
};

} // namespace tanks

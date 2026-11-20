#pragma once

#include <memory>

#include "game/objects/PhysicsObject.h"

namespace tanks
{

// Flies straight ahead (the way its node is rotated) until it hits something solid
// of another team or leaves the bounds. Hitting walls chips their cells away.
class Bullet : public PhysicsObject
{
public:
	static std::unique_ptr<Bullet> Create(cocos2d::Node* InNode);

	// Keys: everything of GameObject plus "speed", "explosion_on_hit" (big bullets explode where they touch),
	// and how it chips walls: "chip_width" (pixels across the flight), "chip_depth" (cells deep),
	// "chip_damage" (taken by every chipped cell), "breaks_stone" (chips indestructible walls too).
	void Configure(const cocos2d::ValueMap& InData) override;

protected:
	void Update(float InDelta) override;

private:
	Bullet() = default;

	bool Build() override;
	void Hit(PhysicsObject* InTarget, const cocos2d::Vec2& InDirection);
	// Chips every wall under the blast at the bullet's position. Returns true if a cell was removed.
	bool ChipWalls(const cocos2d::Vec2& InDirection);

	float MoveSpeed = 500.0f;
	bool bExplodesOnHit = false;
	float ChipWidth = 64.0f;
	int ChipDepth = 2;
	int ChipDamage = 2;
	bool bBreaksStone = false;
};

} // namespace tanks

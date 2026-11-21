#pragma once

#include <memory>

#include "game/objects/PhysicsObject.h"
#include "model/LobFlight.h"

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
	// "chip_damage" (taken by every chipped cell), "breaks_stone" (chips indestructible walls too),
	// "range_tiles" (gone after this distance, 0 = no limit), "bounces" (walls it flies back from before hitting),
	// "lifetime" (seconds until it is gone, 0 = no limit; mines lie still with "speed" 0),
	// "lobbed" (flies over everything for "lob_distance" pixels, then blasts tanks within "blast_radius_tiles").
	void Configure(const cocos2d::ValueMap& InData) override;

protected:
	void Update(float InDelta) override;

private:
	Bullet() = default;

	bool Build() override;
	void Hit(PhysicsObject* InTarget, const cocos2d::Vec2& InDirection);
	// Chips every wall under the blast at the bullet's position. Returns true if a cell was removed.
	bool ChipWalls(const cocos2d::Vec2& InDirection);
	void UpdateLob(float InDelta, const cocos2d::Vec2& InDirection);
	// A lobbed shell lands: every foe tank within the blast radius is hit, walls under it are chipped.
	void Land(const cocos2d::Vec2& InDirection);

	float MoveSpeed = 500.0f;
	bool bExplodesOnHit = false;
	float ChipWidth = 64.0f;
	int ChipDepth = 2;
	int ChipDamage = 2;
	bool bBreaksStone = false;
	float Range = 0.0f;
	float Travelled = 0.0f;
	int BouncesLeft = 0;
	float Lifetime = 0.0f;
	float Age = 0.0f;
	bool bLobbed = false;
	float BlastRadius = 0.0f;
	float BaseScale = 1.0f;
	model::LobFlight Lob;
};

} // namespace tanks

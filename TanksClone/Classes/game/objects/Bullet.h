#pragma once

#include <memory>

#include "game/objects/PhysicsObject.h"
#include "model/LobFlight.h"

namespace game
{
	class Bullet;
	using FBulletHandle = THandle<Bullet>;
/**
 * Flies straight ahead (the way its node is rotated) until it hits something solid
 * of another team or leaves the bounds. Hitting walls chips their cells away.
 */
class Bullet : public PhysicsObject
{
public:
	static FBulletHandle Create(cocos2d::Node* InNode);

	void Configure(const model::JsonValue::Object& InData) override;
	cocos2d::Vec2 GetVelocity() const;

protected:
	void Update(float InDelta) override;

private:
	Bullet() = default;

	bool Build() override;
	void Hit(PhysicsObject* InTarget, const cocos2d::Vec2& InDirection);
	cocos2d::Vec2 GetImpactPoint(const PhysicsObject& InTarget, const cocos2d::Vec2& InDirection) const;
	bool ChipWalls(const cocos2d::Vec2& InDirection);
	void UpdateLob(float InDelta, const cocos2d::Vec2& InDirection);
	bool ResolveClashes(const cocos2d::Vec2& InStart, const cocos2d::Vec2& InEnd, const cocos2d::Vec2& InHalf);
	void Land(const cocos2d::Vec2& InDirection);

	float MoveSpeed = 500.0f;
	bool bExplodesOnHit = false;
	float ChipWidth = 64.0f;
	int ChipDepth = 2;
	int ChipDamage = 2;
	bool bBreaksStone = false;
	float Range = 0.0f;
	std::string EdgeEffect;
	std::string FadeEffect;
	float Travelled = 0.0f;
	int BouncesLeft = 0;
	float Lifetime = 0.0f;
	float Age = 0.0f;
	bool bLobbed = false;
	bool bFriendlyFire = false;
	float FriendlyFireShare = 1.0f;
	bool bClash = false;
	int ClashHealth = 1;
	cocos2d::Vec2 FiredFrom;
	bool bFiredFromKnown = false;
	float BlastRadius = 0.0f;
	float BaseScale = 1.0f;
	model::LobFlight Lob;
};
} // namespace game

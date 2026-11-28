#pragma once

#include "game/objects/GameObject.h"
#include "model/PhysicsTypes.h"
#include "templates/TRefCountedHandle.h"

namespace game
{
	class PhysicsObject;
	using FPhysicsHandle = THandle<PhysicsObject>;

class PhysicsObject : public GameObject
{
public:
	~PhysicsObject() override;

	bool Build() override;
	void Configure(const model::JsonValue::Object& InData) override;

	void SetBounds(const cocos2d::Rect& InBounds);
	const cocos2d::Rect& GetBounds() const;
	bool HasBounds() const;

	cocos2d::Rect GetCollisionRect() const;
	cocos2d::Rect GetCollisionRectAt(const cocos2d::Vec2& InPosition) const;

	virtual void SyncBody() const;

	bool IsCollidedTo(const GameObject* InOther) const;
	bool IsCollidedTo(const FGameObjectHandle& InOther) const;

	virtual bool Blocks(const cocos2d::Rect& InRect) const;
	virtual bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const;

protected:
	void OnDestroying() override;
	model::BodyId GetBodyId() const;
	PhysicsObject* FindBlocker(const cocos2d::Rect& InRect) const;
	PhysicsObject* FindBlocker(const cocos2d::Rect& InRect, const cocos2d::Rect& InCurrent) const;
	bool IsInsideBounds(const cocos2d::Rect& InRect) const;

	model::CollisionChannel GetCollisionChannel() const;

private:
	bool EnsureBody() const;

	mutable model::BodyId PhysicsBody;
	mutable model::Vec2f BodySize;
	cocos2d::Rect Bounds;
	bool bHasBounds = false;
	// Collision box in pixels from collision_width_tiles and collision_height_tiles; zero means the size of the sprite frame.
	model::Vec2f ConfiguredSize;
};
} // namespace game

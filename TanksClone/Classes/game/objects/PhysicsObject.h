#pragma once

#include "game/objects/GameObject.h"

namespace tanks
{

class PhysicsObject : public GameObject
{
public:
	bool Build() override;

	// The area (in the parent's coordinates) the object has to stay in.
	void SetBounds(const cocos2d::Rect& InBounds);
	const cocos2d::Rect& GetBounds() const;
	bool HasBounds() const;

	// The visible part of the sprite (without transparent padding), as an
	// axis-aligned rectangle in the parent's coordinates.
	cocos2d::Rect GetCollisionRect() const;
	// The same rectangle, as if the object stood at InPosition.
	cocos2d::Rect GetCollisionRectAt(const cocos2d::Vec2& InPosition) const;

	bool IsCollidedTo(const GameObject* InOther) const;

	// True if the object stops something occupying InRect. Walls answer with their standing parts only.
	virtual bool Blocks(const cocos2d::Rect& InRect) const;
	// Where along the move a -> b a box of half size InHalf around the moving point first touches the object.
	// OutFraction is the fraction of the move (0 if it starts inside); false if it never does.
	virtual bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const;

protected:
	// First live solid object that overlaps InRect, or nullptr.
	PhysicsObject* FindBlocker(const cocos2d::Rect& InRect) const;
	// The same, but an object that already overlaps InCurrent only counts if the overlap would grow:
	// things stuck inside each other may move apart.
	PhysicsObject* FindBlocker(const cocos2d::Rect& InRect, const cocos2d::Rect& InCurrent) const;
	// True if InRect lies completely inside the bounds (always true without bounds).
	bool IsInsideBounds(const cocos2d::Rect& InRect) const;

private:
	CollisionType CollisionKind = CollisionType::Rect; // only Rect is implemented
	cocos2d::Rect Bounds;
	bool bHasBounds = false;
};

} // namespace tanks

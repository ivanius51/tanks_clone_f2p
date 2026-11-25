#pragma once

#include "game/objects/GameObject.h"

namespace game
{
class PhysicsObject : public GameObject
{
public:
	bool Build() override;

	void SetBounds(const cocos2d::Rect& InBounds);
	const cocos2d::Rect& GetBounds() const;
	bool HasBounds() const;

	cocos2d::Rect GetCollisionRect() const;
	cocos2d::Rect GetCollisionRectAt(const cocos2d::Vec2& InPosition) const;

	bool IsCollidedTo(const GameObject* InOther) const;

	virtual bool Blocks(const cocos2d::Rect& InRect) const;
	virtual bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const;

protected:
	PhysicsObject* FindBlocker(const cocos2d::Rect& InRect) const;
	PhysicsObject* FindBlocker(const cocos2d::Rect& InRect, const cocos2d::Rect& InCurrent) const;
	bool IsInsideBounds(const cocos2d::Rect& InRect) const;

private:
	cocos2d::Rect Bounds;
	bool bHasBounds = false;
};
} // namespace game

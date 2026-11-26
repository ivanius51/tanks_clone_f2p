#pragma once

#include <memory>

#include "game/objects/PhysicsObject.h"
#include "model/WallMask.h"
#include "templates/TContainers.h"

namespace game
{
	class Wall;
	using FWallHandle = THandle<Wall>;
/**
 * A block of the map made of sub-cells (WallMask). Bullets chip cells away from the side they hit;
 * tanks and bullets collide only with the cells that still stand, so a chipped wall can be driven through.
 */
class Wall : public PhysicsObject
{
public:
	void Redraw();
	static FWallHandle Create(cocos2d::Node* InNode);

	void Configure(const model::JsonValue::Object& InData) override;

	bool Blocks(const cocos2d::Rect& InRect) const override;
	bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const override;

	int Chip(const cocos2d::Rect& InArea, model::HitSide InSide, int InDepth, int InDamage, bool bInBreaksStone);

	const model::WallMask& GetMask() const;
	bool IsBase() const;
	const std::string& GetContents() const;
	const std::string& GetDropTable() const;
	bool IsExplosive() const;

protected:
	void OnKilled() override;

private:
	Wall() = default;

	bool Build() override;
	float GetCellSize() const;
	void ThrowDebris(cocos2d::Sprite& InCell);

	model::WallMask Mask{ 4, 1 };
	bool bBase = false;
	std::string Contents;
	std::string DropTable;
	float BlastRadius = 0.0f;
	int BlastDamage = 0;
	TArray<int> ShownHealth;
	float DebrisSeconds = 0.45f;
	float EdgeBleedOpacity = 0.0f;
	bool bBreakBlast = true;
	float DebrisDistance = 0.0f;
};
} // namespace game

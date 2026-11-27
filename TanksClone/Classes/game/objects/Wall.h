#pragma once

#include <memory>

#include "game/PhysicsManager.h"
#include "game/objects/PhysicsObject.h"
#include "model/TileMergeSettings.h"
#include "model/GridBody.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{
class TileMergeView;
}

namespace game
{
	class Wall;
	using FWallHandle = THandle<Wall>;
/**
 * A block of the map made of sub-cells (CellGrid). Bullets chip cells away from the side they hit;
 * tanks and bullets collide only with the cells that still stand, so a chipped wall can be driven through.
 */
class Wall : public PhysicsObject, private IBodyListener
{
public:
	void Redraw();
	static FWallHandle Create(cocos2d::Node* InNode);

	void Configure(const model::JsonValue::Object& InData) override;

	bool HasMergeMaterial() const { return bHasMergeMaterial; }
	model::MergeMaterial GetMergeMaterial() const { return MergeMaterial; }
	void SetMergeView(view::cocos::TileMergeView* InView);

	bool Blocks(const cocos2d::Rect& InRect) const override;
	bool SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const override;

	int Chip(const cocos2d::Rect& InArea, model::HitSide InSide, int InDepth, int InDamage, bool bInBreaksStone);

	const model::CellGrid& GetMask() const;
	bool IsBase() const;
	const std::string& GetContents() const;
	const std::string& GetDropTable() const;
	bool IsExplosive() const;

protected:
	void OnKilled() override;

private:
	Wall() = default;
	~Wall() override;

	void OnCarved(const model::CarveEvent& InEvent) override;
	bool EnsureBody() const;
	void SyncBody() const override;

	bool Build() override;
	float GetCellSize() const;
	bool JustBroke(int InColumn, int InRow) const;
	void ThrowDebris(cocos2d::Sprite& InCell);
	void ThrowBigPiece(cocos2d::Sprite& InCell, const cocos2d::Vec2& InPosition, const cocos2d::Vec2& InHeading);

	void UpdateMergeView();

	model::GridBody Body{ 4, 1 };
	mutable model::BodyId PhysicsBody;
	mutable bool bBodyListening = false;
	view::cocos::TileMergeView* MergeView = nullptr;
	model::MergeMaterial MergeMaterial = model::MergeMaterial::Brick;
	bool bHasMergeMaterial = false;
	int MergeColumn = 0;
	int MergeRow = 0;
	bool bBase = false;
	std::string Contents;
	std::string DropTable;
	float BlastRadius = 0.0f;
	int BlastDamage = 0;
	TArray<model::CellCoord> BrokenCells;
	float DebrisSeconds = 0.45f;
	float EdgeBleedOpacity = 0.0f;
	bool bBreakBlast = true;
	float DebrisDistance = 0.0f;
	float DebrisLingerSeconds = 0.0f;
	float DebrisForwardShare = 0.0f;
	float BigPieceChance = 0.0f;
	float BigPieceStaySeconds = 0.0f;
	cocos2d::Vec2 DebrisHeading;
};
} // namespace game

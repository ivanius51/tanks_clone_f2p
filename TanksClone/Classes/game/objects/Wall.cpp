#include "game/objects/Wall.h"

#include <algorithm>

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ObjectCast.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Tank.h"
#include "services/AudioManager.h"

namespace tanks
{

namespace
{
	constexpr int CellSpriteTag = 7001;
	const cocos2d::Color3B DamagedCellColor(170, 160, 150);
}

std::unique_ptr<Wall> Wall::Create(cocos2d::Node* const InNode)
{
	std::unique_ptr<Wall> CurrentWall(new Wall());
	if ( !CurrentWall->Build() )
	{
		return nullptr;
	}
	CurrentWall->SetVisualNode(InNode);
	return CurrentWall;
}

bool Wall::Build()
{
	const bool bResult = PhysicsObject::Build();
	SetType(ObjectType::Wall);
	return bResult;
}

void Wall::Configure(const cocos2d::ValueMap& InData)
{
	PhysicsObject::Configure(InData);
	Mask = model::WallMask(DataConverter::GetInt(InData, "cells", 4), DataConverter::GetInt(InData, "cell_health", 1));
	bBase = DataConverter::GetBool(InData, "base", false);
	Contents = DataConverter::GetString(InData, "contents", "");
	DropTable = DataConverter::GetString(InData, "drop_table", "");
	BlastRadius = DataConverter::GetFloat(InData, "blast_radius_tiles", 0.0f) * TileSize;
	BlastDamage = DataConverter::GetInt(InData, "blast_damage", 0);
	DebrisSeconds = DataConverter::GetFloat(InData, "debris_seconds", DebrisSeconds);
	DebrisDistance = DataConverter::GetFloat(InData, "debris_distance_tiles", 0.4f) * TileSize;
	const float Lifetime = DataConverter::GetFloat(InData, "lifetime_seconds", 0.0f);
	if ( Lifetime > 0.0f && GetVisualNode() )
	{
		GetVisualNode()->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(Lifetime),
			cocos2d::CallFunc::create([this]() { Destroy(); }), nullptr));
	}
}

const std::string& Wall::GetContents() const
{
	return Contents;
}

bool Wall::IsExplosive() const
{
	return BlastRadius > 0.0f && BlastDamage > 0;
}

const std::string& Wall::GetDropTable() const
{
	return DropTable;
}

bool Wall::IsBase() const
{
	return bBase;
}

const model::WallMask& Wall::GetMask() const
{
	return Mask;
}

float Wall::GetCellSize() const
{
	return GetCollisionRect().size.width / Mask.GetCellsPerSide();
}

bool Wall::Blocks(const cocos2d::Rect& InRect) const
{
	const cocos2d::Rect Box = GetCollisionRect();
	const float Cell = GetCellSize();
	if ( Cell <= 0.0f )
	{
		return false;
	}
	return Mask.Overlaps((InRect.getMinX() - Box.getMinX()) / Cell, (InRect.getMinY() - Box.getMinY()) / Cell,
		(InRect.getMaxX() - Box.getMinX()) / Cell, (InRect.getMaxY() - Box.getMinY()) / Cell);
}

bool Wall::SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const
{
	const cocos2d::Rect Box = GetCollisionRect();
	const float Cell = GetCellSize();
	if ( Cell <= 0.0f )
	{
		return false;
	}
	const cocos2d::Vec2 CurrentOrigin(Box.getMinX(), Box.getMinY());
	const cocos2d::Vec2 From = (InFrom - CurrentOrigin) / Cell;
	const cocos2d::Vec2 To = (InTo - CurrentOrigin) / Cell;
	return Mask.Sweep(From.x, From.y, To.x, To.y, std::max(InHalf.x, InHalf.y) / Cell, OutFraction);
}

int Wall::Chip(const cocos2d::Rect& InArea, const model::HitSide InSide, const int InDepth, const int InDamage, const bool bInBreaksStone)
{
	const cocos2d::Rect Box = GetCollisionRect();
	if ( IsDestroying() || (!IsDestroyable() && !bInBreaksStone) || !InArea.intersectsRect(Box) )
	{
		return 0;
	}
	const bool bOverlapsX = InArea.getMaxX() > Box.getMinX() && InArea.getMinX() < Box.getMaxX();
	const bool bOverlapsY = InArea.getMaxY() > Box.getMinY() && InArea.getMinY() < Box.getMaxY();
	if ( !bOverlapsX || !bOverlapsY )
	{
		return 0;
	}

	const float Cell = GetCellSize();
	const bool bVertical = (InSide == model::HitSide::Bottom || InSide == model::HitSide::Top);
	const float Low = bVertical ? (InArea.getMinX() - Box.getMinX()) / Cell : (InArea.getMinY() - Box.getMinY()) / Cell;
	const float High = bVertical ? (InArea.getMaxX() - Box.getMinX()) / Cell : (InArea.getMaxY() - Box.getMinY()) / Cell;

	const int Removed = Mask.Chip(InSide, Low, High, InDepth, InDamage);
	Redraw();

	if ( Mask.IsEmpty() )
	{
		Kill();
	}
	return Removed;
}

void Wall::Redraw()
{
	auto CurrentSprite = CheckedCast<cocos2d::Sprite>(GetVisualNode());
	if ( !CurrentSprite || !CurrentSprite->getTexture() )
	{
		return;
	}

	const cocos2d::Rect CurrentFrame = CurrentSprite->getTextureRect();
	const int CurrentSize = Mask.GetCellsPerSide();
	const float CellWidth = CurrentFrame.size.width / CurrentSize;
	const float CellHeight = CurrentFrame.size.height / CurrentSize;

	CurrentSprite->setCascadeOpacityEnabled(false);
	CurrentSprite->setOpacity(0);
	while ( cocos2d::Node* Old = CurrentSprite->getChildByTag(CellSpriteTag) )
	{
		Old->removeFromParent();
	}

	const bool bHadCells = ShownHealth.size() == static_cast<size_t>(CurrentSize * CurrentSize);
	ShownHealth.resize(CurrentSize * CurrentSize, 0);
	for ( int Row = 0; Row < CurrentSize; ++Row )
	{
		for ( int Column = 0; Column < CurrentSize; ++Column )
		{
			const int CurrentHealth = Mask.GetCellHealth(Column, Row);
			int& Shown = ShownHealth[Row * CurrentSize + Column];
			const bool bJustBroke = bHadCells && Shown > 0 && CurrentHealth <= 0;
			Shown = CurrentHealth;
			if ( CurrentHealth <= 0 && !bJustBroke )
			{
				continue;
			}
			const cocos2d::Rect Part(CurrentFrame.origin.x + Column * CellWidth, CurrentFrame.origin.y + (CurrentSize - 1 - Row) * CellHeight, CellWidth, CellHeight);
			auto CellSprite = cocos2d::Sprite::createWithTexture(CurrentSprite->getTexture(), Part);
			CellSprite->setPosition((Column + 0.5f) * CellWidth, (Row + 0.5f) * CellHeight);
			// A pixel of overlap hides the seams the camera's fractional positions open between cell sprites.
			CellSprite->setScale((CellWidth + 1.0f) / CellWidth, (CellHeight + 1.0f) / CellHeight);
			if ( bJustBroke )
			{
				CurrentSprite->addChild(CellSprite);
				ThrowDebris(*CellSprite);
				continue;
			}
			if ( CurrentHealth < Mask.GetFullCellHealth() )
			{
				CellSprite->setColor(DamagedCellColor);
			}
			CurrentSprite->addChild(CellSprite, 0, CellSpriteTag);
		}
	}
}

void Wall::ThrowDebris(cocos2d::Sprite& InCell)
{
	cocos2d::Node* const Owner = InCell.getParent();
	cocos2d::Node* const Layer = Owner ? Owner->getParent() : nullptr;
	if ( !Layer )
	{
		InCell.removeFromParent();
		return;
	}
	// Moved to the layer so the piece outlives the wall node.
	const cocos2d::Vec2 Position = Layer->convertToNodeSpace(Owner->convertToWorldSpace(InCell.getPosition()));
	const cocos2d::Vec2 Center = Owner->getPosition();
	InCell.retain();
	InCell.removeFromParent();
	InCell.setPosition(Position);
	Layer->addChild(&InCell, Owner->getLocalZOrder() + 1);
	InCell.release();

	cocos2d::Vec2 Outward = Position - Center;
	Outward = Outward.isZero() ? cocos2d::Vec2(0.0f, 1.0f) : Outward.getNormalized();
	Outward.rotate(cocos2d::Vec2::ZERO, cocos2d::random(-0.6f, 0.6f));
	const float Distance = DebrisDistance * cocos2d::random(0.6f, 1.0f);
	InCell.setColor(DamagedCellColor);
	InCell.runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::create(
			cocos2d::EaseOut::create(cocos2d::MoveBy::create(DebrisSeconds, Outward * Distance), 2.0f),
			cocos2d::RotateBy::create(DebrisSeconds, cocos2d::random(-180.0f, 180.0f)),
			cocos2d::ScaleTo::create(DebrisSeconds, 0.3f),
			cocos2d::FadeOut::create(DebrisSeconds),
			nullptr),
		cocos2d::RemoveSelf::create(),
		nullptr));
}

void Wall::OnKilled()
{
	auto CurrentVisualNode = GetVisualNode();
	if ( CurrentVisualNode )
	{
		while ( auto* const Cell = static_cast<cocos2d::Sprite*>(CurrentVisualNode->getChildByTag(CellSpriteTag)) )
		{
			Cell->setTag(cocos2d::Node::INVALID_TAG);
			ThrowDebris(*Cell);
		}
		effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), IsExplosive());
	}
	if ( CurrentVisualNode && IsExplosive() )
	{
		const cocos2d::Vec2 Center = CurrentVisualNode->getPosition();
		std::vector<Tank*> Victims;
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
		{
			if ( !InTank.IsDestroying() && InTank.GetVisualNode() && InTank.GetVisualNode()->getPosition().distance(Center) <= BlastRadius )
			{
				Victims.push_back(&InTank);
			}
		});
		for ( Tank* const Victim : Victims )
		{
			Victim->ApplyDamage(BlastDamage, Team::Neutral);
		}
	}
	GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::WallBreak);
}

} // namespace tanks

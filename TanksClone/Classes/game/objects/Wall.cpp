#include "game/objects/Wall.h"

#include <algorithm>

#include "core/DataConverter.h"
#include "core/GameContext.h"
#include "core/ObjectCast.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
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

	for ( int Row = 0; Row < CurrentSize; ++Row )
	{
		for ( int Column = 0; Column < CurrentSize; ++Column )
		{
			const int CurrentHealth = Mask.GetCellHealth(Column, Row);
			if ( CurrentHealth <= 0 )
			{
				continue;
			}
			const cocos2d::Rect Part(CurrentFrame.origin.x + Column * CellWidth, CurrentFrame.origin.y + (CurrentSize - 1 - Row) * CellHeight, CellWidth, CellHeight);
			auto CellSprite = cocos2d::Sprite::createWithTexture(CurrentSprite->getTexture(), Part);
			CellSprite->setPosition((Column + 0.5f) * CellWidth, (Row + 0.5f) * CellHeight);
			if ( CurrentHealth < Mask.GetFullCellHealth() )
			{
				CellSprite->setColor(DamagedCellColor);
			}
			CurrentSprite->addChild(CellSprite, 0, CellSpriteTag);
		}
	}
}

void Wall::OnKilled()
{
	if ( auto CurrentVisualNode = GetVisualNode() )
	{
		effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), false);
	}
	GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::WallBreak);
}

} // namespace tanks

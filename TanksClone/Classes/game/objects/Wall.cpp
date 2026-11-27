#include "game/objects/Wall.h"

#include <algorithm>
#include <iterator>

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ObjectCast.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/EventReport.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Tank.h"
#include "services/AudioManager.h"
#include "ui/CocosConvert.h"
#include "ui/TileMergeView.h"
#include "templates/TContainers.h"
#include "ui/RenderOrder.h"

namespace game
{

namespace
{
	constexpr int CellSpriteTag = 7001;
	const cocos2d::Color3B DamagedCellColor(170, 160, 150);
}

FWallHandle Wall::Create(cocos2d::Node* const InNode)
{
	FWallHandle CurrentWall(new Wall());
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
	SetType(EObjectType::Wall);
	return bResult;
}

void Wall::Configure(const model::JsonValue::Object& InData)
{
	PhysicsObject::Configure(InData);
	Body = model::GridBody(DataConverter::GetInt(InData, "cells", 4), DataConverter::GetInt(InData, "cell_health", 1));
	bBase = DataConverter::GetBool(InData, "base", false);
	bHasMergeMaterial = model::TileMergeSettings::FindMaterial(DataConverter::GetString(InData, "merge", ""), MergeMaterial);
	Contents = DataConverter::GetString(InData, "contents", "");
	DropTable = DataConverter::GetString(InData, "drop_table", "");
	BlastRadius = DataConverter::GetFloat(InData, "blast_radius_tiles", 0.0f) * TileSize;
	BlastDamage = DataConverter::GetInt(InData, "blast_damage", 0);
	DebrisSeconds = DataConverter::GetFloat(InData, "debris_seconds", DebrisSeconds);
	bBreakBlast = DataConverter::GetBool(InData, "break_blast", bBreakBlast);
	EdgeBleedOpacity = DataConverter::GetFloat(InData, "edge_bleed_opacity", EdgeBleedOpacity);
	DebrisDistance = DataConverter::GetFloat(InData, "debris_distance_tiles", 0.4f) * TileSize;
	DebrisLingerSeconds = DataConverter::GetFloat(InData, "debris_linger_seconds", 0.35f);
	DebrisForwardShare = DataConverter::GetFloat(InData, "debris_forward_share", 0.7f);
	BigPieceChance = DataConverter::GetFloat(InData, "big_piece_chance", 0.2f);
	BigPieceStaySeconds = DataConverter::GetFloat(InData, "big_piece_stay_seconds", 4.0f);
	const float Lifetime = DataConverter::GetFloat(InData, "lifetime_seconds", 0.0f);
	if ( Lifetime > 0.0f && GetVisualNode() )
	{
		GetVisualNode()->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(Lifetime),
			cocos2d::CallFunc::create([this]() { Destroy(); }), nullptr));
	}
	Redraw();
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

void Wall::SetMergeView(view::cocos::TileMergeView* const InView)
{
	MergeView = InView;
	if ( !MergeView || !GetVisualNode() || MergeView->GetSubdivision() % GetMask().GetCellsPerSide() != 0
		|| !MergeView->ToCell(GetVisualNode()->getPosition(), MergeColumn, MergeRow) )
	{
		MergeView = nullptr;
		return;
	}
	UpdateMergeView();
	Redraw();
}

void Wall::UpdateMergeView()
{
	const int Side = MergeView->GetSubdivision();
	for ( int Row = 0; Row < Side; ++Row )
	{
		for ( int Column = 0; Column < Side; ++Column )
		{
			const int MaskColumn = Column * GetMask().GetCellsPerSide() / Side;
			const int MaskRow = (Side - 1 - Row) * GetMask().GetCellsPerSide() / Side;
			const float Amount = !IsDestroying() && GetMask().IsSolid(MaskColumn, MaskRow) ? 1.0f : 0.0f;
			MergeView->SetCell(MergeColumn * Side + Column, MergeRow * Side + Row, MergeMaterial, Amount);
		}
	}
}

Wall::~Wall()
{
	if ( PhysicsBody.IsValid() )
	{
		GameContext::Get<PhysicsManager>().Forget(PhysicsBody);
		GameContext::Get<PhysicsManager>().GetWorld().DestroyBody(PhysicsBody);
	}
}

void Wall::OnDestroying()
{
	if ( PhysicsBody.IsValid() )
	{
		GameContext::Get<PhysicsManager>().GetWorld().SetEnabled(PhysicsBody, false);
	}
}

void Wall::SyncBody() const
{
	EnsureBody();
}

bool Wall::EnsureBody() const
{
	if ( PhysicsBody.IsValid() )
	{
		return true;
	}
	if ( !GetVisualNode() )
	{
		return false;
	}
	const model::RectF Box = view::cocos::FromCocos(GetCollisionRect());
	model::BodyDef Definition;
	Definition.Type = model::BodyType::Static;
	Definition.BodyShape.Kind = model::ShapeKind::Grid;
	Definition.BodyShape.Size = Box.Size;
	Definition.BodyShape.Anchor = model::Vec2f(0.0f, 0.0f);
	Definition.BodyShape.GridCellsPerSide = Body.GetMask().GetCellsPerSide();
	Definition.BodyShape.GridCellHealth = Body.GetMask().GetFullCellHealth();
	Definition.Profile.ObjectType = model::CollisionChannel::WorldStatic;
	Definition.Position = Box.Origin;
	Definition.Group = static_cast<unsigned>(GetTeam()) + 1;
	PhysicsManager& Physics = GameContext::Get<PhysicsManager>();
	PhysicsBody = Physics.GetWorld().CreateBody(Definition);
	Physics.Listen(PhysicsBody, const_cast<Wall*>(this));
	Physics.Bind(PhysicsBody, const_cast<Wall*>(this));
	return true;
}

const model::CellGrid& Wall::GetMask() const
{
	if ( EnsureBody() )
	{
		return *GameContext::Get<PhysicsManager>().GetWorld().GetGrid(PhysicsBody);
	}
	return Body.GetMask();
}

float Wall::GetCellSize() const
{
	return Body.GetCellSize(view::cocos::FromCocos(GetCollisionRect()));
}

bool Wall::Blocks(const cocos2d::Rect& InRect) const
{
	return EnsureBody() ? GameContext::Get<PhysicsManager>().GetWorld().TestOverlap(PhysicsBody, view::cocos::FromCocos(InRect)) : Body.Blocks(view::cocos::FromCocos(GetCollisionRect()), view::cocos::FromCocos(InRect));
}

bool Wall::SweepHit(const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const cocos2d::Vec2& InHalf, float& OutFraction) const
{
	if ( !EnsureBody() )
	{
		return false;
	}
	return GameContext::Get<PhysicsManager>().GetWorld().SweepBody(PhysicsBody, model::Vec2f(InFrom.x, InFrom.y), model::Vec2f(InTo.x, InTo.y), model::Vec2f(InHalf.x, InHalf.y), OutFraction);
}

int Wall::Chip(const cocos2d::Rect& InArea, const model::HitSide InSide, const int InDepth, const int InDamage, const bool bInBreaksStone)
{
	if ( IsDestroying() || (!IsDestroyable() && !bInBreaksStone) )
	{
		return 0;
	}
	const model::RectF Box = view::cocos::FromCocos(GetCollisionRect());
	if ( !view::cocos::FromCocos(InArea).Intersects(Box) )
	{
		return 0;
	}
	if ( !EnsureBody() )
	{
		return 0;
	}
	PhysicsManager& Physics = GameContext::Get<PhysicsManager>();
	const model::GridCarveResult Result = Physics.GetWorld().Carve(PhysicsBody, view::cocos::FromCocos(InArea), InSide, InDepth, InDamage);
	Physics.Pump();
	const int Removed = Result.Removed;
	if ( Removed > 0 )
	{
		events::Report("wall.chipped");
	}
	return Removed;
}

void Wall::OnCarved(const model::CarveEvent& InEvent)
{
	if ( IsDestroying() )
	{
		return;
	}
	DebrisHeading = cocos2d::Vec2(InEvent.Result.Heading.X, InEvent.Result.Heading.Y);
	BrokenCells = InEvent.Result.BrokenCells;
	Redraw();
	BrokenCells.Clear();
	if ( InEvent.bEmptied )
	{
		events::Report("wall.destroyed");
		Kill();
	}
}

bool Wall::JustBroke(const int InColumn, const int InRow) const
{
	for ( const model::CellCoord& Cell : BrokenCells )
	{
		if ( Cell.Column == InColumn && Cell.Row == InRow )
		{
			return true;
		}
	}
	return false;
}

void Wall::Redraw()
{
	auto CurrentSprite = CheckedCast<cocos2d::Sprite>(GetVisualNode());
	if ( !CurrentSprite || !CurrentSprite->getTexture() )
	{
		return;
	}

	const cocos2d::Rect CurrentFrame = CurrentSprite->getTextureRect();
	const int CurrentSize = GetMask().GetCellsPerSide();
	const float CellWidth = CurrentFrame.size.width / CurrentSize;
	const float CellHeight = CurrentFrame.size.height / CurrentSize;

	CurrentSprite->setCascadeOpacityEnabled(false);
	CurrentSprite->setOpacity(0);
	while ( cocos2d::Node* Old = CurrentSprite->getChildByTag(CellSpriteTag) )
	{
		Old->removeFromParent();
	}
	if ( MergeView )
	{
		UpdateMergeView();
	}

	for ( int Row = 0; Row < CurrentSize; ++Row )
	{
		for ( int Column = 0; Column < CurrentSize; ++Column )
		{
			const int CurrentHealth = GetMask().GetCellHealth(Column, Row);
			const bool bJustBroke = JustBroke(Column, Row);
			if ( CurrentHealth <= 0 && !bJustBroke )
			{
				continue;
			}
			if ( MergeView && !bJustBroke )
			{
				continue;
			}
			const cocos2d::Rect Part(CurrentFrame.origin.x + Column * CellWidth, CurrentFrame.origin.y + (CurrentSize - 1 - Row) * CellHeight, CellWidth, CellHeight);
			auto CellSprite = cocos2d::Sprite::createWithTexture(CurrentSprite->getTexture(), Part);
			CellSprite->setPosition((Column + 0.5f) * CellWidth, (Row + 0.5f) * CellHeight);
			// A pixel of overlap hides the seams the camera's fractional positions open between cell sprites.
			CellSprite->setScale((CellWidth + 1.0f) / CellWidth, (CellHeight + 1.0f) / CellHeight);
			if ( EdgeBleedOpacity > 0.0f && !bJustBroke )
			{
				// A see-through copy one texel larger each way pokes past the block, so neighbouring blocks overlap softly.
				auto Bleed = cocos2d::Sprite::createWithTexture(CurrentSprite->getTexture(), Part);
				Bleed->setPosition(CellSprite->getPosition());
				Bleed->setScale((CellWidth + 2.0f) / CellWidth, (CellHeight + 2.0f) / CellHeight);
				Bleed->setOpacity(static_cast<uint8_t>(EdgeBleedOpacity * 255.0f));
				if ( CurrentHealth < GetMask().GetFullCellHealth() )
				{
					Bleed->setColor(DamagedCellColor);
				}
				CurrentSprite->addChild(Bleed, -1, CellSpriteTag);
			}
			if ( bJustBroke )
			{
				CurrentSprite->addChild(CellSprite);
				ThrowDebris(*CellSprite);
				continue;
			}
			if ( CurrentHealth < GetMask().GetFullCellHealth() )
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
	Layer->addChild(&InCell, view::cocos::GetZOrder(model::RenderLayer::Debris));
	InCell.release();

	cocos2d::Vec2 Around = Position - Center;
	Around = Around.isZero() ? cocos2d::Vec2(0.0f, 1.0f) : Around.getNormalized();
	cocos2d::Vec2 Outward = DebrisHeading.isZero() ? Around : DebrisHeading * DebrisForwardShare + Around * (1.0f - DebrisForwardShare);
	Outward = Outward.isZero() ? Around : Outward.getNormalized();
	Outward.rotate(cocos2d::Vec2::ZERO, cocos2d::random(-0.6f, 0.6f));
	const float Distance = DebrisDistance * cocos2d::random(0.6f, 1.0f);
	if ( cocos2d::random(0.0f, 1.0f) < BigPieceChance )
	{
		ThrowBigPiece(InCell, Position, Outward);
	}
	InCell.setColor(DamagedCellColor);
	InCell.runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::create(
			cocos2d::EaseOut::create(cocos2d::MoveBy::create(DebrisSeconds, Outward * Distance), 2.0f),
			cocos2d::RotateBy::create(DebrisSeconds, cocos2d::random(-180.0f, 180.0f)),
			cocos2d::ScaleTo::create(DebrisSeconds, 0.3f),
			nullptr),
		cocos2d::DelayTime::create(DebrisLingerSeconds),
		cocos2d::FadeOut::create(DebrisSeconds * 0.4f),
		cocos2d::RemoveSelf::create(),
		nullptr));
}

void Wall::ThrowBigPiece(cocos2d::Sprite& InCell, const cocos2d::Vec2& InPosition, const cocos2d::Vec2& InHeading)
{
	struct PieceShape
	{
		float Columns;
		float Rows;
	};
	static constexpr PieceShape Shapes[] = { { 2.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 2.0f }, { 2.0f, 2.0f } };
	const PieceShape Shape = Shapes[cocos2d::random(0, static_cast<int>(std::size(Shapes)) - 1)];
	const cocos2d::Rect Cell = InCell.getTextureRect();
	const cocos2d::Rect Part(Cell.origin.x, Cell.origin.y, Cell.size.width * Shape.Columns, Cell.size.height * Shape.Rows);
	cocos2d::Sprite* const Piece = cocos2d::Sprite::createWithTexture(InCell.getTexture(), Part);
	cocos2d::Node* const Layer = InCell.getParent();
	if ( !Piece || !Layer )
	{
		return;
	}
	Piece->setPosition(InPosition);
	Piece->setScale(InCell.getScaleX());
	Piece->setColor(DamagedCellColor);
	Layer->addChild(Piece, view::cocos::GetZOrder(model::RenderLayer::Debris));
	const float Distance = DebrisDistance * cocos2d::random(0.8f, 1.6f);
	const float Turn = cocos2d::random(-120.0f, 120.0f);
	Piece->runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::create(cocos2d::EaseOut::create(cocos2d::MoveBy::create(DebrisSeconds, InHeading * Distance), 2.0f),
			cocos2d::RotateBy::create(DebrisSeconds, Turn), nullptr),
		cocos2d::DelayTime::create(BigPieceStaySeconds),
		cocos2d::FadeOut::create(DebrisSeconds),
		cocos2d::RemoveSelf::create(),
		nullptr));
}

void Wall::OnKilled()
{
	if ( MergeView )
	{
		UpdateMergeView();
	}
	auto CurrentVisualNode = GetVisualNode();
	if ( CurrentVisualNode )
	{
		while ( auto* const Cell = static_cast<cocos2d::Sprite*>(CurrentVisualNode->getChildByTag(CellSpriteTag)) )
		{
			Cell->setTag(cocos2d::Node::INVALID_TAG);
			ThrowDebris(*Cell);
		}
		if ( bBreakBlast || IsExplosive() )
		{
			effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), IsExplosive());
		}
	}
	if ( CurrentVisualNode && IsExplosive() )
	{
		const cocos2d::Vec2 Center = CurrentVisualNode->getPosition();
		TArray<Tank*> Victims;
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
		{
			if ( !InTank.IsDestroying() && InTank.GetVisualNode() && InTank.GetVisualNode()->getPosition().distance(Center) <= BlastRadius )
			{
				Victims.PushBack(&InTank);
			}
		});
		for ( Tank* const Victim : Victims )
		{
			Victim->ApplyDamage(BlastDamage, ETeam::Neutral);
		}
	}
	GameContext::Get<AudioManager>().Play(ids::sounds::WallBreak);
}

} // namespace game

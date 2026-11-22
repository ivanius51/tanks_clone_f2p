#include "game/TankLook.h"

#include <algorithm>
#include <array>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ObjectCast.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Wall.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/ai/TankAi.h"
#include "game/objects/Tank.h"
#include "model/AiTactic.h"
#include "model/ShotSide.h"
#include "services/ResourceManager.h"

namespace tanks
{

namespace
{
	constexpr int LampTag = 7101;
	constexpr int CracksTag = 7102;
	constexpr int DamageLayerTag = 7103;
	constexpr int DirtTag = 7104;
	constexpr int DirtZ = 3;
	constexpr int HoleZ = 4;
	constexpr int OverlayZ = 5;
	constexpr int LampZ = 6;
	constexpr int GlowZ = -1;
	constexpr int LampTintActionTag = 7110;
	constexpr float FullTurn = 360.0f;
	constexpr GLubyte HiddenOpacity = 128; // a cloaked tank below this leaves no smoke, dust or tracks

	cocos2d::Color3B ReadColor(const model::IConfigNode& InNode)
	{
		std::array<int, 3> Channels{ 255, 255, 255 };
		for ( size_t Index = 0; Index < Channels.size() && Index < InNode.GetItemCount(); ++Index )
		{
			Channels[Index] = static_cast<int>(InNode.GetItem(Index)->ReadNumber().value_or(255.0));
		}
		return cocos2d::Color3B(static_cast<uint8_t>(Channels[0]), static_cast<uint8_t>(Channels[1]), static_cast<uint8_t>(Channels[2]));
	}

	void ReadGroundMark(const model::IConfigNode& InRoot, const std::string_view InKey, GroundMarkSettings& OutMark)
	{
		const model::IConfigNode* const Mark = InRoot.FindChild(InKey);
		if ( !Mark )
		{
			return;
		}
		OutMark.Frame = Mark->GetValueOr("frame", OutMark.Frame);
		OutMark.Spacing = std::max(1.0f, Mark->GetValueOr("spacing_tiles", 0.25f) * TileSize);
		OutMark.Seconds = std::max(0.1f, Mark->GetValueOr("seconds", OutMark.Seconds));
		OutMark.Opacity = static_cast<unsigned char>(std::clamp(Mark->GetValueOr("opacity", 160), 0, 255));
		OutMark.WidthShare = Mark->GetValueOr("width_share", OutMark.WidthShare);
	}

	bool IsSlowed(const Tank& InTank)
	{
		for ( const BuffSet::Active& CurrentBuff : InTank.GetBuffs().GetActive() )
		{
			for ( const StatModifier& Modifier : CurrentBuff.Def.Modifiers )
			{
				if ( Modifier.Stat == StatType::Speed && Modifier.Mul < 1.0f )
				{
					return true;
				}
			}
		}
		return false;
	}

	float GetHealthShare(const Tank& InTank)
	{
		return InTank.GetMaxHealth() > 0 ? static_cast<float>(InTank.GetHealth()) / InTank.GetMaxHealth() : 1.0f;
	}
}

TankLookSettings TankLookSettings::Load()
{
	TankLookSettings Result;
	const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::TankLook, model::ResourceKind::Config);
	const model::IConfigNode& Root = GameContext::Get().GetManager<ConfigReader>().GetConfig(Path);
	if ( const model::IConfigNode* const Lamp = Root.FindChild("lamp") )
	{
		Result.LampOffset = cocos2d::Vec2(Lamp->GetValueOr("offset_x", Result.LampOffset.x), Lamp->GetValueOr("offset_y", Result.LampOffset.y));
		Result.LampScale = Lamp->GetValueOr("scale", Result.LampScale);
		Result.LampTintTime = Lamp->GetValueOr("tint_time", Result.LampTintTime);
		Result.bLampRandomSide = Lamp->GetValueOr("random_side", Result.bLampRandomSide);
		if ( const model::IConfigNode* const Colors = Lamp->FindChild("colors") )
		{
			for ( const std::string& Key : Colors->GetKeys() )
			{
				Result.LampColors[Key] = ReadColor(*Colors->FindChild(Key));
			}
		}
	}
	if ( const model::IConfigNode* const Stages = Root.FindChild("crack_stages") )
	{
		Result.CrackStages.clear();
		for ( size_t Index = 0; Index < Stages->GetItemCount(); ++Index )
		{
			Result.CrackStages.push_back(static_cast<float>(Stages->GetItem(Index)->ReadNumber().value_or(0.0)));
		}
	}
	if ( const model::IConfigNode* const Holes = Root.FindChild("holes") )
	{
		Result.MaxHoles = static_cast<int>(Holes->GetValueOr("max", static_cast<float>(Result.MaxHoles)));
		Result.HoleMergeDistance = Holes->GetValueOr("merge_distance", Result.HoleMergeDistance);
		Result.HoleGrowth = Holes->GetValueOr("growth", Result.HoleGrowth);
		Result.HoleMaxScale = Holes->GetValueOr("max_scale", Result.HoleMaxScale);
		Result.HoleSpread = Holes->GetValueOr("spread", Result.HoleSpread);
		Result.HoleEdge = Holes->GetValueOr("edge", Result.HoleEdge);
	}
	if ( const model::IConfigNode* const Jitter = Root.FindChild("jitter") )
	{
		Result.JitterRotation = Jitter->GetValueOr("rotation", Result.JitterRotation);
		Result.JitterScale = Jitter->GetValueOr("scale", Result.JitterScale);
		Result.bJitterMirror = Jitter->GetValueOr("mirror", Result.bJitterMirror);
	}
	Result.SmokeOffset = cocos2d::Vec2(Root.GetValueOr("smoke_offset_x", Result.SmokeOffset.x), Root.GetValueOr("smoke_offset_y", Result.SmokeOffset.y));
	if ( const model::IConfigNode* const Stages = Root.FindChild("smoke_stages") )
	{
		for ( size_t Index = 0; Index < Stages->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Node = *Stages->GetItem(Index);
			SmokeStage Stage;
			Stage.Below = Node.GetValueOr("below", Stage.Below);
			Stage.Interval = Node.GetValueOr("interval", Stage.Interval);
			Stage.Frame = Node.GetValueOr("frame", Stage.Frame);
			Stage.Scale = Node.GetValueOr("scale", Stage.Scale);
			if ( const model::IConfigNode* const Color = Node.FindChild("color") )
			{
				Stage.Color = ReadColor(*Color);
			}
			Result.SmokeStages.push_back(Stage);
		}
	}
	Result.SmokeRise = Root.GetValueOr("smoke_rise", Result.SmokeRise);
	Result.SmokeTime = Root.GetValueOr("smoke_time", Result.SmokeTime);
	Result.MuzzleEffect = Root.GetValueOr("muzzle_effect", Result.MuzzleEffect);
	Result.MuzzleBack = Root.GetValueOr("muzzle_back_tiles", 0.0f) * TileSize;
	if ( const model::IConfigNode* const Glow = Root.FindChild("shot_glow") )
	{
		Result.ShotGlowSeconds = std::max(0.01f, Glow->GetValueOr("seconds", Result.ShotGlowSeconds));
		Result.ShotGlowOpacity = static_cast<unsigned char>(std::clamp(Glow->GetValueOr("opacity", 0), 0, 255));
	}
	if ( const model::IConfigNode* const Flare = Root.FindChild("muzzle_flare") )
	{
		Result.FlareFrame = Flare->GetValueOr("frame", Result.FlareFrame);
		Result.FlareScale = Flare->GetValueOr("scale", Result.FlareScale);
		Result.FlareSeconds = std::max(0.01f, Flare->GetValueOr("seconds", Result.FlareSeconds));
		Result.FlareOpacity = static_cast<unsigned char>(std::clamp(Flare->GetValueOr("opacity", 170), 0, 255));
		if ( const model::IConfigNode* const Color = Flare->FindChild("color") )
		{
			Result.FlareColor = ReadColor(*Color);
		}
	}
	ReadGroundMark(Root, "track_marks", Result.TrackMark);
	ReadGroundMark(Root, "wheel_marks", Result.WheelMark);
	Result.DustEffect = Root.GetValueOr("dust_effect", Result.DustEffect);
	Result.DustInterval = std::max(0.02f, Root.GetValueOr("dust_interval", Result.DustInterval));
	Result.DustOffset = Root.GetValueOr("dust_offset", Result.DustOffset);
	Result.ImmobileEffect = Root.GetValueOr("immobile_effect", Result.ImmobileEffect);
	Result.ImmobileInterval = std::max(0.02f, Root.GetValueOr("immobile_interval", Result.ImmobileInterval));
	Result.MaskAlphaThreshold = std::clamp(Root.GetValueOr("mask_alpha_threshold", Result.MaskAlphaThreshold), 0.0f, 1.0f);
	if ( const model::IConfigNode* const Wreck = Root.FindChild("wreck") )
	{
		WreckSettings& Out = Result.Wreck;
		if ( const model::IConfigNode* const Color = Wreck->FindChild("flash_color") )
		{
			Out.FlashColor = ReadColor(*Color);
		}
		if ( const model::IConfigNode* const Color = Wreck->FindChild("burnt_color") )
		{
			Out.BurntColor = ReadColor(*Color);
		}
		if ( const model::IConfigNode* const Color = Wreck->FindChild("smoke_color") )
		{
			Out.SmokeColor = ReadColor(*Color);
		}
		Out.BurnSeconds = std::max(0.01f, Wreck->GetValueOr("burn_seconds", Out.BurnSeconds));
		Out.SmokeFrame = Wreck->GetValueOr("smoke_frame", Out.SmokeFrame);
		Out.SmokeScale = Wreck->GetValueOr("smoke_scale", Out.SmokeScale);
		Out.SmokeTime = std::max(0.1f, Wreck->GetValueOr("smoke_time", Out.SmokeTime));
		Out.SmokeRise = Wreck->GetValueOr("smoke_rise", Out.SmokeRise);
		Out.SmokeSeconds = std::max(0.0f, Wreck->GetValueOr("smoke_seconds", Out.SmokeSeconds));
		Out.SmokeInterval = std::max(0.05f, Wreck->GetValueOr("smoke_interval", Out.SmokeInterval));
		Out.PieceGrid = std::max(1, Wreck->GetValueOr("piece_grid", Out.PieceGrid));
		Out.PieceCount = std::max(0, Wreck->GetValueOr("piece_count", Out.PieceCount));
		Out.PieceDistance = Wreck->GetValueOr("piece_distance_tiles", Out.PieceDistance);
		Out.PieceSeconds = std::max(0.01f, Wreck->GetValueOr("piece_seconds", Out.PieceSeconds));
		Out.CorpseHealth = std::max(1, Wreck->GetValueOr("corpse_health", Out.CorpseHealth));
		Out.OverkillShare = std::max(0.01f, Wreck->GetValueOr("overkill_share", Out.OverkillShare));
		Out.TurretFlyTiles = Wreck->GetValueOr("turret_fly_tiles", Out.TurretFlyTiles);
		Out.TurretFlySeconds = std::max(0.01f, Wreck->GetValueOr("turret_fly_seconds", Out.TurretFlySeconds));
		Out.TurretStaySeconds = std::max(0.0f, Wreck->GetValueOr("turret_stay_seconds", Out.TurretStaySeconds));
	}
	return Result;
}

TankLook::TankLook(const TankLookSettings& InSettings)
	: Settings(InSettings)
{
}

float TankLook::GetOverlayScale(const cocos2d::Node& InTankNode) const
{
	cocos2d::SpriteFrame* const Frame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName("damage_dirt");
	return Frame && Frame->getOriginalSize().width > 0.0f ? InTankNode.getContentSize().width / Frame->getOriginalSize().width : 1.0f;
}

void TankLook::Jitter(cocos2d::Sprite& InOverlay, const float InBaseScale, const bool bInFullTurn) const
{
	const float Rotation = bInFullTurn ? cocos2d::RandomHelper::random_real(0.0f, FullTurn)
		: cocos2d::RandomHelper::random_real(-Settings.JitterRotation, Settings.JitterRotation);
	InOverlay.setRotation(Rotation);
	InOverlay.setScale(InBaseScale * cocos2d::RandomHelper::random_real(1.0f - Settings.JitterScale, 1.0f + Settings.JitterScale));
	InOverlay.setFlippedX(Settings.bJitterMirror && cocos2d::RandomHelper::random_int(0, 1) == 1);
}

cocos2d::Node& TankLook::GetDamageLayer(cocos2d::Node& InTankNode) const
{
	if ( cocos2d::Node* const Existing = InTankNode.getChildByTag(DamageLayerTag) )
	{
		return *Existing;
	}
	// The tank's own sprite is the stencil, so cracks, holes and dirt never spill past its outline.
	const auto* const TankSprite = CheckedCast<cocos2d::Sprite>(&InTankNode);
	cocos2d::Sprite* const Stencil = cocos2d::Sprite::createWithSpriteFrame(TankSprite->getSpriteFrame());
	Stencil->setAnchorPoint(cocos2d::Vec2::ZERO);
	cocos2d::ClippingNode* const Layer = cocos2d::ClippingNode::create(Stencil);
	Layer->setAlphaThreshold(Settings.MaskAlphaThreshold);
	Layer->setTag(DamageLayerTag);
	InTankNode.addChild(Layer, DirtZ);
	return *Layer;
}

cocos2d::Sprite* TankLook::EnsureOverlay(cocos2d::Node& InTankNode, const std::string& InFrame, const int InTag, const int InZOrder) const
{
	cocos2d::Node& Layer = GetDamageLayer(InTankNode);
	if ( auto* const Existing = static_cast<cocos2d::Sprite*>(Layer.getChildByTag(InTag)) )
	{
		return Existing;
	}
	cocos2d::Sprite* const Overlay = cocos2d::Sprite::createWithSpriteFrameName(InFrame);
	if ( !Overlay )
	{
		return nullptr;
	}
	const cocos2d::Size TankSize = InTankNode.getContentSize();
	Overlay->setPosition(TankSize.width / 2, TankSize.height / 2);
	Jitter(*Overlay, GetOverlayScale(InTankNode), false);
	Overlay->setTag(InTag);
	Layer.addChild(Overlay, InZOrder);
	return Overlay;
}

void TankLook::Update(Tank& InTank, const float InDelta)
{
	cocos2d::Node* const TankNode = InTank.GetVisualNode();
	if ( !TankNode || InTank.IsDestroying() )
	{
		return;
	}
	const float HealthShare = GetHealthShare(InTank);
	if ( HealthShare >= 1.0f )
	{
		ClearHoles();
	}
	UpdateCracks(*TankNode, HealthShare);
	const bool bSlowed = IsSlowed(InTank);
	if ( bSlowed || GetDamageLayer(*TankNode).getChildByTag(DirtTag) )
	{
		if ( cocos2d::Sprite* const Dirt = EnsureOverlay(*TankNode, "damage_dirt", DirtTag, DirtZ) )
		{
			Dirt->setVisible(bSlowed);
		}
	}
	UpdateLamp(InTank, *TankNode);
	if ( TankNode->getOpacity() < HiddenOpacity )
	{
		bHasMarkPosition = false;
		return;
	}
	UpdateSmoke(*TankNode, HealthShare, InDelta);
	UpdateDust(InTank, *TankNode, InDelta);
	UpdateGroundMarks(InTank, *TankNode);
	UpdateImmobile(InTank, *TankNode, InDelta);
}

void TankLook::UpdateImmobile(Tank& InTank, cocos2d::Node& InTankNode, const float InDelta)
{
	ImmobileTimer -= InDelta;
	if ( Settings.ImmobileEffect.empty() || !InTank.IsImmobilized() || ImmobileTimer > 0.0f )
	{
		return;
	}
	ImmobileTimer = Settings.ImmobileInterval;
	bImmobileLeft = !bImmobileLeft;
	const cocos2d::Vec2 Facing = InTank.GetDirection();
	const cocos2d::Vec2 Outward = bImmobileLeft ? cocos2d::Vec2(-Facing.y, Facing.x) : cocos2d::Vec2(Facing.y, -Facing.x);
	const float Half = InTankNode.getContentSize().width * InTankNode.getScaleX() * 0.5f;
	effects::Play(Settings.ImmobileEffect, InTankNode.getParent(), InTankNode.getPosition() + Outward * Half, -Outward);
}

void TankLook::OnShot(Tank& InTank, const cocos2d::Vec2& InMuzzle, const cocos2d::Vec2& InDirection, const effects::EffectStyle& InFlash) const
{
	if ( cocos2d::Node* const TankNode = InTank.GetVisualNode() )
	{
		// face_shot effects turn against the given direction, so the flame gets the reverse to point forward.
		const cocos2d::Vec2 Flash = InMuzzle - InDirection.getNormalized() * Settings.MuzzleBack;
		effects::Play(Settings.MuzzleEffect, TankNode->getParent(), Flash, -InDirection, InFlash);
		if ( cocos2d::Sprite* const Flare = cocos2d::Sprite::createWithSpriteFrameName(Settings.FlareFrame) )
		{
			Flare->setPosition(Flash);
			Flare->setScale(Settings.FlareScale * std::max(InFlash.Width, InFlash.Length));
			Flare->setColor(Settings.FlareColor);
			Flare->setOpacity(Settings.FlareOpacity);
			Flare->setBlendFunc(cocos2d::BlendFunc::ADDITIVE);
			TankNode->getParent()->addChild(Flare, ZEffect + 1);
			Flare->runAction(cocos2d::Sequence::create(cocos2d::FadeOut::create(Settings.FlareSeconds), cocos2d::RemoveSelf::create(), nullptr));
		}
		PlayShotGlow(*TankNode);
	}
}

void TankLook::PlayShotGlow(cocos2d::Node& InTankNode) const
{
	auto* const Hull = CheckedCast<cocos2d::Sprite>(&InTankNode);
	if ( Settings.ShotGlowOpacity == 0 || !Hull->getSpriteFrame() )
	{
		return;
	}
	auto* const Glow = cocos2d::Sprite::createWithSpriteFrame(Hull->getSpriteFrame());
	Glow->setPosition(Hull->getContentSize() / 2.0f);
	Glow->setOpacity(Settings.ShotGlowOpacity);
	Glow->setBlendFunc(cocos2d::BlendFunc::ADDITIVE);
	Hull->addChild(Glow, 1);
	Glow->runAction(cocos2d::Sequence::create(cocos2d::FadeOut::create(Settings.ShotGlowSeconds), cocos2d::RemoveSelf::create(), nullptr));
}

void TankLook::PlayDestroyed(Tank& InTank) const
{
	auto* const TankSprite = CheckedCast<cocos2d::Sprite>(InTank.GetVisualNode());
	cocos2d::Node* const Layer = TankSprite ? TankSprite->getParent() : nullptr;
	if ( !Layer || !TankSprite->getSpriteFrame() )
	{
		return;
	}
	const WreckSettings& Wreck = Settings.Wreck;
	cocos2d::ValueVector Position{ cocos2d::Value(TankSprite->getPosition().x), cocos2d::Value(TankSprite->getPosition().y) };
	cocos2d::ValueMap Overrides;
	Overrides["position"] = Position;
	Overrides["cell_health"] = GetCorpseHealth(InTank);
	const std::string Config = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::TankWreck, model::ResourceKind::Config);
	Wall* const Corpse = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Wall>(Config, Layer, Overrides);
	auto* const Hull = Corpse ? CheckedCast<cocos2d::Sprite>(Corpse->GetVisualNode()) : nullptr;
	if ( !Hull )
	{
		ThrowPieces(*TankSprite);
		return;
	}
	cocos2d::SpriteFrame* const WreckFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(InTank.GetTexturePath() + "_wreck");
	Hull->setSpriteFrame(WreckFrame ? WreckFrame : TankSprite->getSpriteFrame());
	Hull->setRotation(TankSprite->getRotation());
	Hull->setScale(TankSprite->getScaleX(), TankSprite->getScaleY());
	Corpse->Redraw();
	// Cells drawn after a hit are children: they take the burnt tint too.
	Hull->setCascadeColorEnabled(true);
	Hull->setColor(Wreck.FlashColor);

	const float SmokeRise = Wreck.SmokeRise;
	const float SmokeTime = Wreck.SmokeTime;
	const std::string SmokeFrame = Wreck.SmokeFrame;
	const float SmokeScale = Wreck.SmokeScale;
	const cocos2d::Color3B SmokeColor = Wreck.SmokeColor;
	// The callback holds no TankLook: it runs after the tank and its look are gone.
	auto Smoke = cocos2d::CallFunc::create([Hull, SmokeRise, SmokeTime, SmokeColor, SmokeFrame, SmokeScale]()
	{
		cocos2d::Sprite* const Puff = cocos2d::Sprite::createWithSpriteFrameName(SmokeFrame);
		if ( !Puff || !Hull->getParent() )
		{
			return;
		}
		Puff->setPosition(Hull->getPosition());
		Puff->setRotation(cocos2d::RandomHelper::random_real(0.0f, FullTurn));
		Puff->setColor(SmokeColor);
		Puff->setScale(SmokeScale);
		Hull->getParent()->addChild(Puff, ZEffect);
		Puff->runAction(cocos2d::Sequence::create(
			cocos2d::Spawn::create(cocos2d::MoveBy::create(SmokeTime, cocos2d::Vec2(0.0f, SmokeRise)),
				cocos2d::ScaleBy::create(SmokeTime, 2.0f), cocos2d::FadeOut::create(SmokeTime), nullptr),
			cocos2d::RemoveSelf::create(), nullptr));
	});
	const int Puffs = std::max(1, static_cast<int>((Wreck.BurnSeconds + Wreck.SmokeSeconds) / Wreck.SmokeInterval));
	Hull->runAction(cocos2d::Repeat::create(cocos2d::Sequence::create(Smoke, cocos2d::DelayTime::create(Wreck.SmokeInterval), nullptr), Puffs));
	Hull->runAction(cocos2d::TintTo::create(Wreck.BurnSeconds, Wreck.BurntColor));
	ThrowPieces(*TankSprite);
	ThrowTurret(*TankSprite);
}

int TankLook::GetCorpseHealth(const Tank& InTank) const
{
	const WreckSettings& Wreck = Settings.Wreck;
	const float Limit = Wreck.OverkillShare * std::max(1, InTank.GetMaxHealth());
	const float Left = 1.0f - std::min(1.0f, InTank.GetLastOverkill() / Limit);
	return std::max(1, static_cast<int>(std::lround(Wreck.CorpseHealth * Left)));
}

void TankLook::ThrowTurret(cocos2d::Sprite& InTankSprite) const
{
	auto* const Turret = static_cast<cocos2d::Sprite*>(InTankSprite.getChildByName("turret"));
	cocos2d::Node* const Layer = InTankSprite.getParent();
	if ( !Turret || !Turret->getSpriteFrame() )
	{
		return;
	}
	const WreckSettings& Wreck = Settings.Wreck;
	const cocos2d::Vec2 Center = Turret->getContentSize() / 2;
	cocos2d::Sprite* const Flying = cocos2d::Sprite::createWithSpriteFrame(Turret->getSpriteFrame());
	Flying->setPosition(Layer->convertToNodeSpace(Turret->convertToWorldSpace(Center)));
	Flying->setRotation(InTankSprite.getRotation() + Turret->getRotation());
	const float Scale = InTankSprite.getScaleX();
	Flying->setScale(Scale);
	Flying->setColor(Wreck.FlashColor);
	Layer->addChild(Flying, ZEffect);
	cocos2d::Vec2 Away(0.0f, 1.0f);
	Away.rotate(cocos2d::Vec2::ZERO, cocos2d::RandomHelper::random_real(0.0f, 6.2832f));
	const float Half = Wreck.TurretFlySeconds / 2;
	Flying->runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::create(
			cocos2d::EaseOut::create(cocos2d::MoveBy::create(Wreck.TurretFlySeconds, Away * Wreck.TurretFlyTiles * TileSize), 1.5f),
			cocos2d::RotateBy::create(Wreck.TurretFlySeconds, cocos2d::RandomHelper::random_real(-360.0f, 360.0f)),
			// Up into the air and back down.
			cocos2d::Sequence::create(cocos2d::EaseOut::create(cocos2d::ScaleTo::create(Half, Scale * 1.5f), 2.0f),
				cocos2d::EaseIn::create(cocos2d::ScaleTo::create(Half, Scale), 2.0f), nullptr),
			cocos2d::TintTo::create(Wreck.TurretFlySeconds, Wreck.BurntColor),
			nullptr),
		cocos2d::DelayTime::create(Wreck.TurretStaySeconds),
		cocos2d::FadeOut::create(Wreck.PieceSeconds),
		cocos2d::RemoveSelf::create(),
		nullptr));
}

void TankLook::ThrowPieces(cocos2d::Sprite& InTankSprite) const
{
	const WreckSettings& Wreck = Settings.Wreck;
	cocos2d::SpriteFrame* const Frame = InTankSprite.getSpriteFrame();
	cocos2d::Node* const Layer = InTankSprite.getParent();
	// A frame stored rotated in the atlas can't be cut by plain sub-rects.
	if ( Frame->isRotated() || Wreck.PieceCount <= 0 )
	{
		return;
	}
	const cocos2d::Rect Whole = Frame->getRect();
	const cocos2d::Size Content = InTankSprite.getContentSize();
	const int Grid = Wreck.PieceGrid;
	std::vector<int> Cells(Grid * Grid);
	for ( int Index = 0; Index < Grid * Grid; ++Index )
	{
		Cells[Index] = Index;
	}
	for ( int Index = Grid * Grid - 1; Index > 0; --Index )
	{
		std::swap(Cells[Index], Cells[cocos2d::RandomHelper::random_int(0, Index)]);
	}
	const cocos2d::Vec2 Center = InTankSprite.getPosition();
	for ( int Index = 0; Index < std::min(Wreck.PieceCount, Grid * Grid); ++Index )
	{
		const int Column = Cells[Index] % Grid;
		const int Row = Cells[Index] / Grid;
		const cocos2d::Rect Part(Whole.origin.x + Column * Whole.size.width / Grid, Whole.origin.y + Row * Whole.size.height / Grid,
			Whole.size.width / Grid, Whole.size.height / Grid);
		cocos2d::Sprite* const Piece = cocos2d::Sprite::createWithTexture(Frame->getTexture(), Part);
		const cocos2d::Vec2 Local((Column + 0.5f) * Content.width / Grid, (Grid - Row - 0.5f) * Content.height / Grid);
		const cocos2d::Vec2 Position = Layer->convertToNodeSpace(InTankSprite.convertToWorldSpace(Local));
		Piece->setPosition(Position);
		Piece->setRotation(InTankSprite.getRotation());
		Piece->setScale(InTankSprite.getScaleX());
		Piece->setColor(Wreck.BurntColor);
		Layer->addChild(Piece, ZEffect);
		cocos2d::Vec2 Outward = Position - Center;
		Outward = Outward.isZero() ? cocos2d::Vec2(0.0f, 1.0f) : Outward.getNormalized();
		Outward.rotate(cocos2d::Vec2::ZERO, cocos2d::RandomHelper::random_real(-0.5f, 0.5f));
		const float Distance = Wreck.PieceDistance * TileSize * cocos2d::RandomHelper::random_real(0.6f, 1.0f);
		Piece->runAction(cocos2d::Sequence::create(
			cocos2d::Spawn::create(
				cocos2d::EaseOut::create(cocos2d::MoveBy::create(Wreck.PieceSeconds, Outward * Distance), 2.0f),
				cocos2d::RotateBy::create(Wreck.PieceSeconds, cocos2d::RandomHelper::random_real(-270.0f, 270.0f)),
				cocos2d::ScaleBy::create(Wreck.PieceSeconds, 0.4f),
				cocos2d::FadeOut::create(Wreck.PieceSeconds),
				nullptr),
			cocos2d::RemoveSelf::create(),
			nullptr));
	}
}

void TankLook::UpdateGroundMarks(Tank& InTank, cocos2d::Node& InTankNode)
{
	const GroundMarkSettings& Mark = InTank.IsWheeled() ? Settings.WheelMark : Settings.TrackMark;
	const cocos2d::Vec2 Position = InTankNode.getPosition();
	if ( !bHasMarkPosition )
	{
		LastMarkPosition = Position;
		bHasMarkPosition = true;
		return;
	}
	const cocos2d::Vec2 Step = Position - LastMarkPosition;
	const float Length = Step.length();
	if ( Mark.Frame.empty() || Length < Mark.Spacing || !InTankNode.getParent() )
	{
		return;
	}
	LastMarkPosition = Position;
	if ( Length > Mark.Spacing * 4.0f )
	{
		return;
	}
	auto* const Segment = cocos2d::Sprite::createWithSpriteFrameName(Mark.Frame);
	if ( !Segment )
	{
		return;
	}
	const cocos2d::Size FrameSize = Segment->getContentSize();
	const float Width = InTankNode.getContentSize().width * InTankNode.getScaleX() * Mark.WidthShare;
	Segment->setPosition(Position - Step * 0.5f);
	Segment->setScale(Width / FrameSize.width, (Length + 1.0f) / FrameSize.height);
	Segment->setRotation(CC_RADIANS_TO_DEGREES(std::atan2(Step.x, Step.y)));
	Segment->setOpacity(Mark.Opacity);
	InTankNode.getParent()->addChild(Segment, ZBackground + 1);
	Segment->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(Mark.Seconds * 0.6f),
		cocos2d::FadeOut::create(Mark.Seconds * 0.4f), cocos2d::RemoveSelf::create(), nullptr));
}

void TankLook::UpdateDust(Tank& InTank, cocos2d::Node& InTankNode, const float InDelta)
{
	DustTimer -= InDelta;
	if ( Settings.DustEffect.empty() || !InTank.IsRunning() || InTank.IsBlocked() || DustTimer > 0.0f )
	{
		return;
	}
	DustTimer = Settings.DustInterval;
	const float Behind = InTankNode.getContentSize().height * InTankNode.getScaleY() * Settings.DustOffset;
	effects::Play(Settings.DustEffect, InTankNode.getParent(), InTankNode.getPosition() - InTank.GetDirection() * Behind);
}

void TankLook::UpdateCracks(cocos2d::Node& InTankNode, const float InHealthShare)
{
	int Stage = 0;
	while ( Stage < static_cast<int>(Settings.CrackStages.size()) && InHealthShare < Settings.CrackStages[Stage] )
	{
		++Stage;
	}
	if ( Stage == CrackStage )
	{
		return;
	}
	CrackStage = Stage;
	const std::string Frame = "damage_cracks_" + std::to_string(std::max(1, Stage));
	cocos2d::Sprite* const Cracks = EnsureOverlay(InTankNode, Frame, CracksTag, OverlayZ);
	if ( !Cracks )
	{
		return;
	}
	if ( Stage > 0 )
	{
		Cracks->setSpriteFrame(Frame);
	}
	Cracks->setVisible(Stage > 0);
}

void TankLook::AddHit(Tank& InTank, const cocos2d::Vec2& InShotDirection)
{
	cocos2d::Node* const TankNode = InTank.GetVisualNode();
	if ( !TankNode || !TankNode->getParent() || InShotDirection.isZero() )
	{
		return;
	}
	const cocos2d::Size TankSize = TankNode->getContentSize();
	const model::HitFace Face = model::ShotSideClassifier::ClassifyFace({ InTank.GetDirection().x, InTank.GetDirection().y }, { InShotDirection.x, InShotDirection.y });
	// The sprite is drawn facing up: the front is +Y in its own space, whatever the tank's rotation.
	cocos2d::Vec2 Normal(0.0f, 1.0f);
	switch ( Face )
	{
		case model::HitFace::Front: Normal = cocos2d::Vec2(0.0f, 1.0f); break;
		case model::HitFace::Back: Normal = cocos2d::Vec2(0.0f, -1.0f); break;
		case model::HitFace::Left: Normal = cocos2d::Vec2(-1.0f, 0.0f); break;
		case model::HitFace::Right: Normal = cocos2d::Vec2(1.0f, 0.0f); break;
	}
	const float Along = cocos2d::RandomHelper::random_real(-Settings.HoleSpread, Settings.HoleSpread);
	const cocos2d::Vec2 Local(TankSize.width * (0.5f + Normal.x * Settings.HoleEdge + Normal.y * Along),
		TankSize.height * (0.5f + Normal.y * Settings.HoleEdge + Normal.x * Along));

	const float BaseScale = GetOverlayScale(*TankNode);
	const float MergeDistance = Settings.HoleMergeDistance * TankSize.width;
	for ( cocos2d::Sprite* const Hole : Holes )
	{
		if ( Hole->getPosition().distance(Local) < MergeDistance )
		{
			Hole->setScale(std::min(Hole->getScale() * Settings.HoleGrowth, BaseScale * Settings.HoleMaxScale));
			return;
		}
	}
	if ( static_cast<int>(Holes.size()) >= Settings.MaxHoles )
	{
		return;
	}
	cocos2d::Sprite* const Hole = cocos2d::Sprite::createWithSpriteFrameName("damage_hole");
	if ( !Hole )
	{
		return;
	}
	Hole->setPosition(Local);
	Jitter(*Hole, BaseScale, true);
	GetDamageLayer(*TankNode).addChild(Hole, HoleZ);
	Holes.push_back(Hole);
}

void TankLook::ClearHoles()
{
	for ( cocos2d::Sprite* const Hole : Holes )
	{
		Hole->removeFromParent();
	}
	Holes.clear();
}

void TankLook::UpdateLamp(Tank& InTank, cocos2d::Node& InTankNode)
{
	if ( !InTank.GetAi() )
	{
		return;
	}
	auto* Lamp = static_cast<cocos2d::Sprite*>(InTankNode.getChildByTag(LampTag));
	if ( !Lamp )
	{
		Lamp = cocos2d::Sprite::createWithSpriteFrameName("lamp");
		if ( !Lamp )
		{
			return;
		}
		Lamp->setTag(LampTag);
		Lamp->setCascadeColorEnabled(true);
		if ( cocos2d::Sprite* const Glow = cocos2d::Sprite::createWithSpriteFrameName("lamp_glow") )
		{
			Glow->setPosition(Lamp->getContentSize().width / 2, Lamp->getContentSize().height / 2);
			Glow->setBlendFunc(cocos2d::BlendFunc::ADDITIVE);
			Lamp->addChild(Glow, GlowZ);
		}
		const cocos2d::Size TankSize = InTankNode.getContentSize();
		const bool bMirrored = Settings.bLampRandomSide && cocos2d::RandomHelper::random_int(0, 1) == 1;
		const float OffsetX = bMirrored ? 1.0f - Settings.LampOffset.x : Settings.LampOffset.x;
		Lamp->setPosition(TankSize.width * OffsetX, TankSize.height * Settings.LampOffset.y);
		Lamp->setScale(Settings.LampScale * GetOverlayScale(InTankNode));
		Lamp->setColor(LampColor);
		InTankNode.addChild(Lamp, LampZ);
	}
	const auto Found = Settings.LampColors.find(std::string(model::GetTacticName(InTank.GetAi()->GetTactic())));
	const cocos2d::Color3B Target = Found != Settings.LampColors.end() ? Found->second : cocos2d::Color3B::WHITE;
	if ( Target == LampColor )
	{
		return;
	}
	LampColor = Target;
	Lamp->stopActionByTag(LampTintActionTag);
	cocos2d::Action* const Tint = cocos2d::TintTo::create(Settings.LampTintTime, Target);
	Tint->setTag(LampTintActionTag);
	Lamp->runAction(Tint);
}

void TankLook::UpdateSmoke(cocos2d::Node& InTankNode, const float InHealthShare, const float InDelta)
{
	SmokeTimer -= InDelta;
	const SmokeStage* Stage = nullptr;
	for ( const SmokeStage& Candidate : Settings.SmokeStages )
	{
		if ( InHealthShare < Candidate.Below )
		{
			Stage = &Candidate;
		}
	}
	if ( !Stage || SmokeTimer > 0.0f || !InTankNode.getParent() )
	{
		return;
	}
	SmokeTimer = Stage->Interval;
	cocos2d::Sprite* const Puff = cocos2d::Sprite::createWithSpriteFrameName(Stage->Frame);
	if ( !Puff )
	{
		return;
	}
	const cocos2d::Size TankSize = InTankNode.getContentSize();
	const cocos2d::Vec2 Engine(TankSize.width * Settings.SmokeOffset.x, TankSize.height * Settings.SmokeOffset.y);
	Puff->setPosition(InTankNode.getParent()->convertToNodeSpace(InTankNode.convertToWorldSpace(Engine)));
	Jitter(*Puff, Stage->Scale, true);
	Puff->setColor(Stage->Color);
	InTankNode.getParent()->addChild(Puff, InTankNode.getLocalZOrder() + 1);
	Puff->runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::create(cocos2d::MoveBy::create(Settings.SmokeTime, cocos2d::Vec2(0.0f, Settings.SmokeRise)),
			cocos2d::ScaleBy::create(Settings.SmokeTime, 2.0f), cocos2d::FadeOut::create(Settings.SmokeTime), nullptr),
		cocos2d::RemoveSelf::create(), nullptr));
}

} // namespace tanks

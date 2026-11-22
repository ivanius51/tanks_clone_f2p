#include "game/Effects.h"

#include <algorithm>
#include <string>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/ConfigNode.h"
#include "services/ResourceManager.h"

namespace tanks
{
namespace effects
{

namespace
{
	constexpr float EffectScale = 2.0f;
	constexpr float DefaultFrameDelay = 0.05f;
	constexpr int DefaultFrameCount = 1;
	constexpr std::string_view BigExplosionId = "explosion_big";
	constexpr std::string_view SmallExplosionId = "explosion_small";
}

void Play(const std::string_view InEffectId, cocos2d::Node* const InParent, const cocos2d::Vec2& InPosition, const cocos2d::Vec2& InShotDirection, const EffectStyle& InStyle)
{
	if ( !InParent || InEffectId.empty() )
	{
		return;
	}
	ResourceManager& Resources = GameContext::Get().GetManager<ResourceManager>();
	const std::string Path = Resources.ResolvePath(ids::configs::Effects, model::ResourceKind::Config);
	const model::IConfigNode* const Definition = GameContext::Get().GetManager<ConfigReader>().GetConfig(Path).FindChild(InEffectId);
	if ( !Definition )
	{
		CCLOGERROR("effects: no effect %s in %s", std::string(InEffectId).c_str(), Path.c_str());
		return;
	}

	const std::string FramePattern = Definition->GetValueOr("frames", std::string());
	const int FrameCount = Definition->GetValueOr("count", DefaultFrameCount);
	auto CurrentAnimation = cocos2d::Animation::create();
	for ( int Index = 1; Index <= FrameCount; ++Index )
	{
		if ( cocos2d::SpriteFrame* const CurrentFrame = Resources.FindFrame(cocos2d::StringUtils::format(FramePattern.c_str(), Index)) )
		{
			CurrentAnimation->addSpriteFrame(CurrentFrame);
		}
	}
	if ( CurrentAnimation->getFrames().empty() )
	{
		return;
	}
	CurrentAnimation->setDelayPerUnit(Definition->GetValueOr("delay", DefaultFrameDelay) / std::max(0.05f, InStyle.Speed));

	auto CurrentSprite = cocos2d::Sprite::createWithSpriteFrame(CurrentAnimation->getFrames().front()->getSpriteFrame());
	CurrentSprite->setPosition(InPosition);
	const float Scale = Definition->GetValueOr("scale", EffectScale);
	CurrentSprite->setScale(Scale * InStyle.Width, Scale * InStyle.Length);
	CurrentSprite->setColor(InStyle.Tint);
	if ( Definition->GetValueOr("face_shot", false) && !InShotDirection.isZero() )
	{
		// The frame flies up; Cocos rotation is clockwise from up, so it turns to fly back against the shot.
		const cocos2d::Vec2 Back = -InShotDirection;
		CurrentSprite->setRotation(CC_RADIANS_TO_DEGREES(std::atan2(Back.x, Back.y)));
		CurrentSprite->setAnchorPoint(cocos2d::Vec2(0.5f, 0.15f));
	}
	InParent->addChild(CurrentSprite, ZEffect);
	CurrentSprite->runAction(cocos2d::Sequence::create(cocos2d::Animate::create(CurrentAnimation), cocos2d::RemoveSelf::create(), nullptr));
}

void Explosion(cocos2d::Node* const InParent, const cocos2d::Vec2& InPosition, const bool bInBig)
{
	Play(bInBig ? BigExplosionId : SmallExplosionId, InParent, InPosition);
}

void RepairPulse(cocos2d::Node* const InParent, const cocos2d::Vec2& InPosition)
{
	if ( !InParent )
	{
		return;
	}
	auto Ring = cocos2d::DrawNode::create();
	Ring->drawCircle(cocos2d::Vec2::ZERO, TileSize * 0.6f, 0.0f, 32, false, cocos2d::Color4F(0.4f, 1.0f, 0.5f, 1.0f));
	Ring->drawSolidCircle(cocos2d::Vec2::ZERO, TileSize * 0.6f, 0.0f, 32, cocos2d::Color4F(0.4f, 1.0f, 0.5f, 0.25f));
	Ring->setPosition(InPosition);
	Ring->setScale(0.5f);
	InParent->addChild(Ring, ZEffect);
	Ring->runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::create(cocos2d::ScaleTo::create(0.4f, 1.3f), cocos2d::FadeOut::create(0.4f), nullptr),
		cocos2d::RemoveSelf::create(),
		nullptr));
}

void SpawnFlash(cocos2d::Node* const InParent, const cocos2d::Vec2& InPosition)
{
	if ( !InParent )
	{
		return;
	}

	cocos2d::SpriteFrame* const CurrentFrame = GameContext::Get().GetManager<ResourceManager>().FindFrame("spawn_marker");
	if ( !CurrentFrame )
	{
		return;
	}

	auto CurrentSprite = cocos2d::Sprite::createWithSpriteFrame(CurrentFrame);
	CurrentSprite->setPosition(InPosition);
	CurrentSprite->setScale(EffectScale);
	CurrentSprite->setOpacity(0);
	InParent->addChild(CurrentSprite, ZEffect);
	CurrentSprite->runAction(cocos2d::RotateBy::create(0.5f, 90.0f));
	CurrentSprite->runAction(cocos2d::Sequence::create(
		cocos2d::FadeTo::create(0.25f, 230),
		cocos2d::Spawn::create(cocos2d::FadeOut::create(0.25f), cocos2d::ScaleTo::create(0.25f, EffectScale * 0.4f), nullptr),
		cocos2d::RemoveSelf::create(),
		nullptr));
}

} // namespace effects
} // namespace tanks

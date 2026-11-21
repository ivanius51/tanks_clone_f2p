#include "game/Effects.h"

#include <string>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "services/ResourceManager.h"

namespace tanks
{
namespace effects
{

namespace
{
	constexpr float ExplosionFrameDelay = 0.05f;
	constexpr float EffectScale = 2.0f;
	constexpr int BigExplosionFrames = 5;
	constexpr int SmallExplosionFrames = 3;
}

void Explosion(cocos2d::Node* const InParent, const cocos2d::Vec2& InPosition, const bool bInBig)
{
	if ( !InParent )
	{
		return;
	}

	ResourceManager& Resources = GameContext::Get().GetManager<ResourceManager>();
	auto CurrentAnimation = cocos2d::Animation::create();
	const int FrameCount = bInBig ? BigExplosionFrames : SmallExplosionFrames;
	for ( int Index = 1; Index <= FrameCount; ++Index )
	{
		if ( cocos2d::SpriteFrame* const CurrentFrame = Resources.FindFrame("boom" + std::to_string(Index)) )
		{
			CurrentAnimation->addSpriteFrame(CurrentFrame);
		}
	}
	if ( CurrentAnimation->getFrames().empty() )
	{
		return;
	}
	CurrentAnimation->setDelayPerUnit(ExplosionFrameDelay);

	auto CurrentSprite = cocos2d::Sprite::create();
	CurrentSprite->setSpriteFrame(CurrentAnimation->getFrames().front()->getSpriteFrame());
	CurrentSprite->setPosition(InPosition);
	CurrentSprite->setScale(bInBig ? EffectScale * 1.5f : EffectScale);
	InParent->addChild(CurrentSprite, ZEffect);
	CurrentSprite->runAction(cocos2d::Sequence::create(cocos2d::Animate::create(CurrentAnimation), cocos2d::RemoveSelf::create(), nullptr));
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

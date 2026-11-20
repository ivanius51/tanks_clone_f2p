#include "game/Effects.h"

#include "core/GameConstants.h"

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

	auto SpriteCache = cocos2d::SpriteFrameCache::getInstance();
	auto CurrentAnimation = cocos2d::Animation::create();
	const int FrameCount = bInBig ? BigExplosionFrames : SmallExplosionFrames;
	for ( int Index = 1; Index <= FrameCount; ++Index )
	{
		auto CurrentFrame = SpriteCache->getSpriteFrameByName("boom" + std::to_string(Index));
		if ( CurrentFrame )
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

void SpawnFlash(cocos2d::Node* const InParent, const cocos2d::Vec2& InPosition)
{
	if ( !InParent )
	{
		return;
	}

	auto CurrentFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName("spawn_marker");
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

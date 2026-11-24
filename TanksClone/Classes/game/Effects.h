#pragma once

#include <string_view>

#include "cocos2d.h"

namespace tanks
{
namespace effects
{
void Explosion(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, bool bInBig);

/** Per-use changes to an effect: size across and along the shot, playback speed, tint. */
struct EffectStyle
{
	float Width = 1.0f;
	float Length = 1.0f;
	float Speed = 1.0f;
	cocos2d::Color3B Tint = cocos2d::Color3B::WHITE;
};

void Play(std::string_view InEffectId, cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, const cocos2d::Vec2& InShotDirection = cocos2d::Vec2::ZERO,
	const EffectStyle& InStyle = EffectStyle());

void SpawnFlash(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition);

void RepairPulse(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition);
} // namespace effects
} // namespace tanks

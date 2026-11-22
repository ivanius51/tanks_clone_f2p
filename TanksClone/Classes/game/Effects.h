#pragma once

#include <string_view>

#include "cocos2d.h"

namespace tanks
{
namespace effects
{

// Short animation that removes itself when done. Does nothing if InParent is null.
// A big explosion is for tanks, a small one for hits and walls.
void Explosion(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, bool bInBig);

// Plays an effect from configs/effects.json; one marked face_shot is turned to fly back against InShotDirection.
void Play(std::string_view InEffectId, cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, const cocos2d::Vec2& InShotDirection = cocos2d::Vec2::ZERO);

// Brief flash where a tank is about to appear.
void SpawnFlash(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition);

// A green ring that grows and fades where a tank is repaired.
void RepairPulse(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition);

} // namespace effects
} // namespace tanks

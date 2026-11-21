#pragma once

#include "cocos2d.h"

namespace tanks::view::cocos
{

/**
 * A fade through black between two scenes. Unlike the engine's TransitionFade, it keeps both scenes at the
 * design offset when the fade ends (the engine resets them to the origin for one frame, which shifts the screen
 * left on a wide window until the next scene recentres itself).
 */
class FadeTransition final : public cocos2d::TransitionScene
{
public:
	static FadeTransition* create(float InDuration, cocos2d::Scene* InScene);

	void onEnter() override;
	void onExit() override;

private:
	void Finish();

	cocos2d::LayerColor* Fade = nullptr;
};

} // namespace tanks::view::cocos

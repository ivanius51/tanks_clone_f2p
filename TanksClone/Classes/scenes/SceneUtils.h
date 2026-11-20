#pragma once

#include "cocos2d.h"

namespace tanks::view::cocos
{

// Replaces the running scene with a fade. Does nothing if InScene is null.
inline void SwitchScene(cocos2d::Scene* const InScene)
{
	if ( InScene )
	{
		cocos2d::Director::getInstance()->replaceScene(cocos2d::TransitionFade::create(0.25f, InScene));
	}
}

} // namespace tanks::view::cocos
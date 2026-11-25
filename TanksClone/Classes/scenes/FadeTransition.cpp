#include "scenes/FadeTransition.h"

#include "core/GameContext.h"
#include "services/InputManager.h"
#include "ui/CocosConvert.h"
#include "ui/ScreenFit.h"

namespace game::view::cocos
{

FadeTransition* FadeTransition::create(const float InDuration, cocos2d::Scene* const InScene)
{
	auto Result = new (std::nothrow) FadeTransition();
	if ( Result && Result->initWithDuration(InDuration, InScene) )
	{
		Result->autorelease();
		return Result;
	}
	delete Result;
	return nullptr;
}

void FadeTransition::onEnter()
{
	TransitionScene::onEnter();
	_inScene->setVisible(false);
	Fade = cocos2d::LayerColor::create(cocos2d::Color4B::BLACK);
	Fade->setOpacity(0);
	addChild(Fade, 2);
	Fade->runAction(cocos2d::Sequence::create(
		cocos2d::FadeIn::create(_duration / 2),
		cocos2d::CallFunc::create([this]() { hideOutShowIn(); }),
		cocos2d::FadeOut::create(_duration / 2),
		cocos2d::CallFunc::create([this]() { Finish(); }),
		nullptr));
}

void FadeTransition::onExit()
{
	TransitionScene::onExit();
	// The engine turns events off for the fade, so key releases during it never arrive.
	GameContext::Get<InputManager>().ReleaseAll();
	if ( Fade )
	{
		Fade->removeFromParent();
		Fade = nullptr;
	}
}

void FadeTransition::Finish()
{
	finish();
	const cocos2d::Vec2 Offset = ToCocos(GetScreenLayout().GetDesignOffset());
	PlaceScene(_inScene, Offset);
	PlaceScene(_outScene, Offset);
}

} // namespace game::view::cocos

#include "scenes/SceneUtils.h"

#include "core/GameContext.h"
#include "scenes/FadeTransition.h"
#include "services/UiStyle.h"

namespace game::view::cocos
{

void SwitchScene(cocos2d::Scene* const InScene)
{
	if ( InScene )
	{
		const float Duration = GameContext::Get<UiStyle>().GetWidgets().SceneFadeTime;
		cocos2d::Director::getInstance()->replaceScene(FadeTransition::create(Duration, InScene));
	}
}

} // namespace game::view::cocos

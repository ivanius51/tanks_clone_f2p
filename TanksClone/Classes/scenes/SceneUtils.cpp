#include "scenes/SceneUtils.h"

#include "core/GameContext.h"
#include "scenes/FadeTransition.h"
#include "services/UiStyle.h"

namespace tanks::view::cocos
{

void SwitchScene(cocos2d::Scene* const InScene)
{
	if ( InScene )
	{
		const float Duration = GameContext::Get().GetManager<UiStyle>().GetWidgets().SceneFadeTime;
		cocos2d::Director::getInstance()->replaceScene(FadeTransition::create(Duration, InScene));
	}
}

} // namespace tanks::view::cocos

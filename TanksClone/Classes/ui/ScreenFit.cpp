#include "ui/ScreenFit.h"

#include "core/GameConstants.h"
#include "ui/CocosConvert.h"

namespace game::view::cocos
{

model::ScreenLayout GetScreenLayout()
{
	const model::Vec2f Design(DesignWidth, DesignHeight);
	const cocos2d::GLView* const View = cocos2d::Director::getInstance()->getOpenGLView();
	return model::ScreenLayout(Design, View ? FromCocos(View->getFrameSize()) : Design);
}

void ApplyScreenFit()
{
	auto Director = cocos2d::Director::getInstance();
	cocos2d::GLView* const View = Director->getOpenGLView();
	if ( !View )
	{
		return;
	}
	// The visible area has the window's aspect, so EXACT_FIT scales both axes alike and leaves no bars.
	const model::Vec2f Visible = GetScreenLayout().GetVisibleSize();
	View->setDesignResolutionSize(Visible.X, Visible.Y, ResolutionPolicy::EXACT_FIT);
	Director->setViewport();
	Director->getEventDispatcher()->dispatchCustomEvent(ScreenLayoutChangedEvent);
}

void WatchWindowResizes()
{
	cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(cocos2d::GLViewImpl::EVENT_WINDOW_RESIZED,
		[](cocos2d::EventCustom*) { ApplyScreenFit(); });
}

void OnScreenLayoutChanged(cocos2d::Node* const InOwner, const TFunction<void(const model::ScreenLayout&), UiCallbackInlineSize>& InHandler)
{
	InHandler(GetScreenLayout());
	auto Listener = cocos2d::EventListenerCustom::create(ScreenLayoutChangedEvent, [InHandler](cocos2d::EventCustom*) { InHandler(GetScreenLayout()); });
	InOwner->getEventDispatcher()->addEventListenerWithSceneGraphPriority(Listener, InOwner);
}

void PlaceScene(cocos2d::Scene* const InScene, const cocos2d::Vec2& InOffset)
{
	// The engine refreshes a node's cached transform only when a value changes: a scene drawn inside a fade keeps
	// the fade's transform and shows shifted, so the position is changed for real.
	InScene->setPosition(InOffset + cocos2d::Vec2(1.0f, 0.0f));
	InScene->setPosition(InOffset);
}

void CenterOnScreen(cocos2d::Scene* const InScene)
{
	OnScreenLayoutChanged(InScene, [InScene](const model::ScreenLayout& InLayout) { PlaceScene(InScene, ToCocos(InLayout.GetDesignOffset())); });
}

void CoverScreen(cocos2d::Node* const InLayer)
{
	OnScreenLayoutChanged(InLayer, [InLayer](const model::ScreenLayout& InLayout)
	{
		const model::RectF Visible = InLayout.GetVisibleRect();
		InLayer->setPosition(ToCocos(Visible.Origin));
		InLayer->setContentSize(ToCocosSize(Visible.Size));
	});
}

} // namespace game::view::cocos

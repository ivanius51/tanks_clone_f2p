#pragma once

#include <type_traits>

#include "cocos2d.h"

#include "ui/CocosConvert.h"
#include "ui/ScreenFit.h"

namespace tanks::view::cocos
{

enum class SceneKind
{
	Loading = 1,
	Menu,
	Game,
	Upgrades,
	Editor,
	Settings,
	WidgetEditor
};

// Base of every game scene. The scene kind is stored in the node tag, so code that only has a
// cocos2d::Scene* (the running scene, which can also be a transition) checks the tag and
// then uses static_cast. No dynamic_cast needed.
// The scene keeps its design area in the middle of the window, whatever the window's aspect ratio.
template <SceneKind TKind>
class TypedScene : public cocos2d::Scene
{
public:
	static constexpr SceneKind Kind = TKind;

	// A finished transition puts its scenes back at the origin, and a projection change resets the camera.
	void onEnterTransitionDidFinish() override
	{
		cocos2d::Scene::onEnterTransitionDidFinish();
		PlaceScene(this, ToCocos(GetScreenLayout().GetDesignOffset()));
	}

protected:
	TypedScene()
	{
		setTag(SceneTag(TKind));
		CenterOnScreen(this);
	}

public:
	static constexpr int SceneTag(const SceneKind InKind) { return SceneTagBase + static_cast<int>(InKind); }

private:
	static constexpr int SceneTagBase = 0x5C000;
};

// Returns InScene as TScene when it is a scene of that kind, otherwise nullptr.
template <typename TScene>
TScene* SceneCast(cocos2d::Scene* const InScene)
{
	static_assert(std::is_base_of<TypedScene<TScene::Kind>, TScene>::value, "TScene must derive from TypedScene");
	if ( InScene && InScene->getTag() == TypedScene<TScene::Kind>::SceneTag(TScene::Kind) )
	{
		return static_cast<TScene*>(InScene);
	}
	return nullptr;
}

} // namespace tanks::view::cocos

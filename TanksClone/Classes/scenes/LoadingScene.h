#pragma once

#include <memory>

#include "cocos2d.h"

#include "scenes/TypedScene.h"
#include "services/ResourceLoadJob.h"

namespace tanks::view::cocos
{
/**
 * Shown at startup while the sprites, configs, profile and sounds are loaded, and before a campaign level
 * while the resources of that level are loaded with a progress bar.
 */
class LoadingScene : public TypedScene<SceneKind::Loading>
{
public:
	static cocos2d::Scene* CreateScene();
	static cocos2d::Scene* CreateForLevel(int InLevelIndex, float InMinSeconds = 0.0f);

	bool init() override;
	void onEnter() override;
	void onEnterTransitionDidFinish() override;
	void update(float InDelta) override;

	CREATE_FUNC(LoadingScene);

private:
	void LoadResources();
	void StartLevelLoad();
	void DrawProgress(float InProgress);

	int LevelIndex = -1;
	float MinSeconds = 0.0f;
	float ElapsedSeconds = 0.0f;
	int LoggedStep = -1;
	std::unique_ptr<ResourceLoadJob> Job;
	cocos2d::DrawNode* Bar = nullptr;
};
} // namespace tanks::view::cocos

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "cocos2d.h"

#include "core/GameSession.h"
#include "game/ai/PlayerBot.h"
#include "game/EnemySpawner.h"
#include "game/LevelLoader.h"
#include "game/SceneScript.h"
#include "model/DropRoller.h"
#include "model/MapCamera.h"
#include "model/ResourceId.h"
#include "scenes/TypedScene.h"
#include "services/ResourceHolder.h"
#include "model/TileMergeSettings.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "templates/TUniquePtr.h"

namespace game
{
class GameObject;
class Tank;
}

namespace game::view::cocos
{
class TileMergeView;
class LayoutWidget;
/** One level being played: the map, the player, the enemies, the HUD, pause and result screens. */
class GameScene : public TypedScene<SceneKind::Game>
{
public:
	enum class Origin
	{
		Menu,
		Editor
	};

	static cocos2d::Scene* CreateScene(int InLevelIndex);
	static cocos2d::Scene* CreateScene(int InLevelIndex, ResourceHolder InPreloaded);
	static cocos2d::Scene* CreateCustomScene(Origin InOrigin);
	static cocos2d::Scene* CreateCampaignScene(const std::string& InCampaignId, int InPosition);

	static cocos2d::Scene* CreateForTest(const LevelData& InLevel, bool bInWithBot = true);

	float GetMapScale() const { return PlayArea ? PlayArea->getScale() : 1.0f; }
	cocos2d::Vec2 GetMapOffset() const { return PlayArea ? PlayArea->getPosition() : cocos2d::Vec2::ZERO; }
	void AddPauseButton(const std::string& InTitle, const TFunction<void(), UiCallbackInlineSize>& InAction);
	bool IsFinished() const;
	bool WasWon() const;
	float GetElapsed() const;
	const model::GameSession& GetSession() const;
	const EnemySpawner* GetSpawner() const;
	const PlayerBot* GetBot() const;
	TFunction<void(bool), UiCallbackInlineSize> OnFinished;

	void onEnterTransitionDidFinish() override;
	void onExit() override;
	void update(float InDelta) override;

private:
	static constexpr int CustomLevel = -1;

	struct CampaignRef
	{
		std::string Id;
		int Position = 0;
	};

	static GameScene* create(int InLevelIndex, Origin InOrigin, const LevelData* InProvided = nullptr, bool bInWithBot = true, ResourceHolder* InPreloaded = nullptr,
		const CampaignRef* InCampaign = nullptr);
	bool init(int InLevelIndex, Origin InOrigin, const LevelData* InProvided, bool bInWithBot, ResourceHolder* InPreloaded, const CampaignRef* InCampaign);

	void BuildLevel();
	void SpawnObject(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition, int InZOrder);
	model::TileMergeSettings LoadTileMergeSettings() const;
	void BuildTileDebugPanel();
	void ApplyTuning();
	TileMergeView* CreateFloorMerge(const model::TileMergeSettings& InSettings, unsigned InSeed) const;
	GameObject* SpawnWall(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition);
	void SpawnPlayer(const cocos2d::Vec2& InPosition);
	void ShowBanner(const std::string& InText);

	void OnEnemyKilled(GameObject& InEnemy, bool bInCountsForWin);
	void DropPickup(const Tank& InEnemy);
	void SpawnPickup(std::string_view InConfig, const cocos2d::Vec2& InPosition);
	void SpawnCrate(const cocos2d::Vec2& InPosition, std::string_view InContents);
	void SpawnSplitParts(const Tank& InEnemy);
	void SpawnBeside(const Tank& InEnemy, std::string_view InConfig, int InCount, float InSpacingTiles);
	void SkipResultDelay();
	void SetUpEnemy(Tank& InEnemy, bool bInCountsForWin);
	void OnPlayerKilled();
	void UpdateAiDebug();
	void UpdateCamera();
	void Finish(bool bInWon, float InDelay);
	void ShowResult(bool bInWon);

	void TogglePause();
	void SetPaused(bool bInPaused);
	void ShowPauseMenu();
	void ShowOptions();
	void HidePauseLayers();
	void OnBackKey();
	void Restart();
	void Leave();

	bool IsCustom() const;
	bool IsCampaignLevel() const { return !Campaign.Id.empty(); }

	int LevelIndex = 0;
	CampaignRef Campaign;
	Origin OpenedFrom = Origin::Menu;
	TArray<std::pair<std::string, TFunction<void(), UiCallbackInlineSize>>> ExtraPauseButtons;
	LevelData Level;
	TUniquePtr<ResourceHolder> Resources;
	std::string Tileset;
	model::GameSession Session;
	cocos2d::Rect Playfield;

	cocos2d::Node* World = nullptr;
	TileMergeView* WallMerge = nullptr;
	TileMergeView* FloorMerge = nullptr;
	LayoutWidget* TileDebugPanel = nullptr;
	model::TileMergeSettings TuneSettings;
	bool bTuneReady = false;
	cocos2d::ClippingRectangleNode* PlayClip = nullptr;
	cocos2d::Node* PlayArea = nullptr;
	model::Vec2f CameraFocus;
	cocos2d::Node* PauseOverlay = nullptr;
	cocos2d::Node* OptionsOverlay = nullptr;
	TUniquePtr<EnemySpawner> Spawner;
	TUniquePtr<SceneScript> Script;
	cocos2d::Vec2 PlayerSpawn;
	TMap<char, TArray<cocos2d::Vec2>> EnemySpawns;

	bool bPaused = false;
	bool bShowAiDebug = false;
	cocos2d::Node* AiDebugLayer = nullptr;
	TArray<cocos2d::Label*> AiDebugLabels;
	cocos2d::Label* AiDebugPanel = nullptr;
	bool bFinished = false;
	std::string LoseReasonKey = "RESULT_TANK_LOST";
	bool bWon = false;
	float Elapsed = 0.0f;
	TUniquePtr<PlayerBot> Bot;
	bool bTestRun = false;
	Tank* Player = nullptr;
	bool bPlayerHurt = false;
	int PlayerHealth = 0;
	model::DropRoller Drops;
};
} // namespace game::view::cocos

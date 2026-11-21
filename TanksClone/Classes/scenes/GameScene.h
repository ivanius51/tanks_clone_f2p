#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cocos2d.h"

#include "core/GameSession.h"
#include "game/ai/PlayerBot.h"
#include "game/EnemySpawner.h"
#include "game/LevelLoader.h"
#include "model/DropRoller.h"
#include "model/ResourceId.h"
#include "scenes/TypedScene.h"
#include "services/ResourceHolder.h"

namespace tanks
{
class GameObject;
class Tank;
}

namespace tanks::view::cocos
{

// One level being played: the map, the player, the enemies, the HUD, pause and result screens.
class GameScene : public TypedScene<SceneKind::Game>
{
public:
	// Where "back" leads from this level.
	enum class Origin
	{
		Menu,
		Editor
	};

	// A level that ships with the game (0 = the first). Returns nullptr if it cannot be loaded.
	static cocos2d::Scene* CreateScene(int InLevelIndex);
	// With the level's resources already loaded (see LoadingScene).
	static cocos2d::Scene* CreateScene(int InLevelIndex, ResourceHolder InPreloaded);
	// The level made in the editor. Nothing is earned or saved while playing it.
	static cocos2d::Scene* CreateCustomScene(Origin InOrigin);

	// Plays the given level with the bot at the controls (for the automatic tests):
	// nothing is saved and no result screen is shown, onFinished is called instead.
	// Without the bot the test drives the player's tank itself.
	static cocos2d::Scene* CreateForTest(const LevelData& InLevel, bool bInWithBot = true);
	bool IsFinished() const;
	bool WasWon() const;
	float GetElapsed() const;
	const model::GameSession& GetSession() const;
	const EnemySpawner* GetSpawner() const;
	// The bot at the controls of a test run; nullptr in a normal game.
	const PlayerBot* GetBot() const;
	std::function<void(bool)> OnFinished;

	// A level runs at the player's frame rate limit, the menus at the menu rate.
	void onEnterTransitionDidFinish() override;
	void onExit() override;
	void update(float InDelta) override;

private:
	static constexpr int CustomLevel = -1;

	static GameScene* create(int InLevelIndex, Origin InOrigin, const LevelData* InProvided = nullptr, bool bInWithBot = true, ResourceHolder* InPreloaded = nullptr);
	bool init(int InLevelIndex, Origin InOrigin, const LevelData* InProvided, bool bInWithBot, ResourceHolder* InPreloaded);

	void BuildLevel();
	void SpawnObject(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition, int InZOrder);
	// A wall drawn with the frames of the level's tileset.
	void SpawnWall(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition);
	void SpawnPlayer(const cocos2d::Vec2& InPosition);
	void ShowBanner(const std::string& InText);

	void OnEnemyKilled(GameObject& InEnemy);
	void DropPickup(const Tank& InEnemy);
	// A splitter leaves smaller tanks side by side; they count towards the enemies to kill.
	void SpawnSplitParts(const Tank& InEnemy);
	void SetUpEnemy(Tank& InEnemy);
	void OnPlayerKilled();
	void Finish(bool bInWon, float InDelay);
	void ShowResult(bool bInWon);

	void TogglePause();
	void SetPaused(bool bInPaused);
	void ShowPauseMenu();
	void ShowOptions();
	void HidePauseLayers();
	// Esc / Back: closes the options back to the pause menu, otherwise pauses or resumes.
	void OnBackKey();
	void Restart();
	void Leave();

	bool IsCustom() const;

	int LevelIndex = 0;
	Origin OpenedFrom = Origin::Menu;
	LevelData Level;
	std::unique_ptr<ResourceHolder> Resources;
	std::string Tileset;
	model::GameSession Session;
	cocos2d::Rect Playfield;

	cocos2d::Node* World = nullptr;
	cocos2d::Node* PauseOverlay = nullptr;
	cocos2d::Node* OptionsOverlay = nullptr;
	std::unique_ptr<EnemySpawner> Spawner;
	cocos2d::Vec2 PlayerSpawn;
	std::map<char, std::vector<cocos2d::Vec2>> EnemySpawns; // spawner tile -> its positions

	bool bPaused = false;
	bool bFinished = false;
	bool bWon = false;
	float Elapsed = 0.0f;
	std::unique_ptr<PlayerBot> Bot;
	bool bTestRun = false;
	Tank* Player = nullptr; // cleared when it is killed
	bool bPlayerHurt = false;
	int PlayerHealth = 0;
	model::DropRoller Drops;
};

} // namespace tanks::view::cocos

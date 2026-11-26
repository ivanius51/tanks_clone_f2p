#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "game/LevelLoader.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"

namespace game
{
class Tank;

/**
 * Runs the waves of a level: for every wave it takes the listed spawners, lets each one
 * send its share of enemies (chosen from the spawner's pool: its allowed classes / enemies
 * minus the restrictions) and waits until the wave is destroyed before starting the next.
 */
class EnemySpawner
{
public:
	EnemySpawner(cocos2d::Node* InWorld, const cocos2d::Rect& InBounds, const LevelData& InLevel,
		const TMap<char, std::vector<cocos2d::Vec2>>& InPoints);

	void Update(float InDelta);

	bool IsFinished() const;
	int GetTotal() const;
	int GetSpawned() const;
	bool IsDone() const;
	void SendReplacement();
	int GetWaveCount() const;

	TFunction<void(Tank&)> OnSpawned;
	TFunction<void(int, int)> OnWaveStarted;

private:
	enum class State
	{
		Delay,
		Spawning,
		WaitClear,
		Done
	};

	int CountAliveEnemies() const;
	bool IsFree(const cocos2d::Vec2& InPoint) const;
	bool IsReserved(const cocos2d::Vec2& InPoint) const;
	void FinishSpawn(const cocos2d::Vec2& InPoint, std::string_view InEnemyId, char InSpawner);
	void StartWave();
	void TrySpawnNext();
	void SpawnAt(const cocos2d::Vec2& InPoint, std::string_view InEnemyId);
	const std::vector<std::string>& GetPool(char InSpawner);

	cocos2d::Node* World;
	cocos2d::Rect Bounds;
	LevelData Level;
	TMap<char, std::vector<cocos2d::Vec2>> Points;
	TMap<char, std::vector<std::string>> Pools;

	State CurrentState = State::Delay;
	size_t WaveIndex = 0;
	std::vector<char> Queue;
	std::vector<cocos2d::Vec2> Reserved;
	int Spawned = 0;
	int Pending = 0;
	float Timer = 0.0f;
};
} // namespace game

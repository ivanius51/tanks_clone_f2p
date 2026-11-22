#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "game/LevelLoader.h"

namespace tanks
{

class Tank;

// Runs the waves of a level: for every wave it takes the listed spawners, lets each one
// send its share of enemies (chosen from the spawner's pool: its allowed classes / enemies
// minus the restrictions) and waits until the wave is destroyed before starting the next.
class EnemySpawner
{
public:
	// InPoints: where every spawner character stands on the map (in the world's coordinates).
	EnemySpawner(cocos2d::Node* InWorld, const cocos2d::Rect& InBounds, const LevelData& InLevel,
		const std::map<char, std::vector<cocos2d::Vec2>>& InPoints);

	// Called by the scene every frame.
	void Update(float InDelta);

	// All waves have been sent (the last enemies may still be alive).
	bool IsFinished() const;
	int GetTotal() const;
	int GetSpawned() const;
	bool IsDone() const;
	// An enemy lost without a player kill: one more comes in its place.
	void SendReplacement();
	int GetWaveCount() const;

	// Called with every new enemy, so that the scene can follow it.
	std::function<void(Tank&)> OnSpawned;
	// Called when a wave starts: wave number (from 1) and the number of waves.
	std::function<void(int, int)> OnWaveStarted;

private:
	enum class State
	{
		Delay,     // waiting for the wave to start
		Spawning,  // sending enemies
		WaitClear, // everything is sent, waiting until the enemies are destroyed
		Done
	};

	int CountAliveEnemies() const;
	bool IsFree(const cocos2d::Vec2& InPoint) const;
	// A point with a spawn flash on it: the enemy is about to appear there.
	bool IsReserved(const cocos2d::Vec2& InPoint) const;
	// Called when the flash ends: the enemy appears if the point is still free, otherwise it is queued again.
	void FinishSpawn(const cocos2d::Vec2& InPoint, std::string_view InEnemyId, char InSpawner);
	void StartWave();
	void TrySpawnNext();
	void SpawnAt(const cocos2d::Vec2& InPoint, std::string_view InEnemyId);
	const std::vector<std::string>& GetPool(char InSpawner);

	cocos2d::Node* World;
	cocos2d::Rect Bounds;
	LevelData Level;
	std::map<char, std::vector<cocos2d::Vec2>> Points;
	std::map<char, std::vector<std::string>> Pools;

	State CurrentState = State::Delay;
	size_t WaveIndex = 0;
	std::vector<char> Queue; // spawners still to send an enemy in the current wave
	std::vector<cocos2d::Vec2> Reserved;
	int Spawned = 0;
	int Pending = 0;         // spawn flashes that have not turned into tanks yet
	float Timer = 0.0f;
};

} // namespace tanks

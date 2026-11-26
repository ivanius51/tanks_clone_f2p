#include "game/EnemySpawner.h"

#include <algorithm>
#include <iterator>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/Effects.h"
#include "game/EnemyCatalog.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "model/Log.h"
#include "model/Random.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	constexpr float FlashDuration = 0.5f;
	constexpr float DownDegrees = 180.0f;
	// The check area is a bit smaller than the tile: walls that only touch the tile
	// (even with a corner) must not block the spawn point.
	constexpr float FreeAreaMargin = 6.0f;

	template <typename T>
	void Shuffle(TArray<T>& InItems)
	{
		for ( int Index = static_cast<int>(InItems.Size()) - 1; Index > 0; --Index )
		{
			std::swap(InItems[Index], InItems[model::GameRandom().Int(0, Index)]);
		}
	}
}

EnemySpawner::EnemySpawner(cocos2d::Node* const InWorld, const cocos2d::Rect& InBounds, const LevelData& InLevel,
	const TMap<char, TArray<cocos2d::Vec2>>& InPoints)
	: World(InWorld)
	, Bounds(InBounds)
	, Level(InLevel)
	, Points(InPoints)
{
	if ( Level.Waves.IsEmpty() )
	{
		CurrentState = State::Done;
	}
}

bool EnemySpawner::IsFinished() const
{
	return CurrentState == State::WaitClear || CurrentState == State::Done;
}

int EnemySpawner::GetTotal() const
{
	return LevelLoader::GetTotalEnemies(Level);
}

int EnemySpawner::GetSpawned() const
{
	return Spawned;
}

bool EnemySpawner::IsDone() const
{
	return CurrentState == State::Done;
}

void EnemySpawner::SendReplacement()
{
	if ( Points.IsEmpty() || Level.Waves.IsEmpty() )
	{
		return;
	}
	const auto PointIt = Points.begin() + model::GameRandom().Int(0, static_cast<int>(Points.Size()) - 1);
	Queue.PushBack(PointIt->Key);
	if ( CurrentState != State::Spawning )
	{
		WaveIndex = std::min(WaveIndex, Level.Waves.Size() - 1);
		CurrentState = State::Spawning;
		Timer = Level.Waves[WaveIndex].Interval;
	}
}

int EnemySpawner::GetWaveCount() const
{
	return static_cast<int>(Level.Waves.Size());
}

void EnemySpawner::Update(const float InDelta)
{
	if ( CurrentState == State::Done )
	{
		return;
	}

	const Wave& CurrentWave = Level.Waves[WaveIndex];
	Timer += InDelta;

	switch ( CurrentState )
	{
		case State::Delay:
			if ( Timer >= CurrentWave.Delay )
			{
				StartWave();
			}
			break;

		case State::Spawning:
			if ( Queue.IsEmpty() && Pending == 0 )
			{
				CurrentState = State::WaitClear;
			}
			else if ( Timer >= CurrentWave.Interval && !Queue.IsEmpty() && CountAliveEnemies() + Pending < CurrentWave.MaxAlive )
			{
				TrySpawnNext();
			}
			break;

		case State::WaitClear:
			if ( CountAliveEnemies() == 0 && Pending == 0 )
			{
				++WaveIndex;
				Timer = 0.0f;
				CurrentState = WaveIndex < Level.Waves.Size() ? State::Delay : State::Done;
			}
			break;

		default:
			break;
	}
}

void EnemySpawner::StartWave()
{
	const Wave& CurrentWave = Level.Waves[WaveIndex];

	Queue.Clear();
	for ( const auto& CurrentEntry : CurrentWave.Entries )
	{
		Queue.Insert(Queue.end(), std::max(0, CurrentEntry.Count), CurrentEntry.Spawner);
	}
	Shuffle(Queue);

	CurrentState = State::Spawning;
	Timer = CurrentWave.Interval; // the first enemy comes at once

	if ( OnWaveStarted )
	{
		OnWaveStarted(static_cast<int>(WaveIndex) + 1, GetWaveCount());
	}
}

void EnemySpawner::TrySpawnNext()
{
	const char CurrentSpawner = Queue.Back();

	const auto& Pool = GetPool(CurrentSpawner);
	const TArray<cocos2d::Vec2>* const SpawnerPoints = Points.Find(CurrentSpawner);
	if ( Pool.IsEmpty() || !SpawnerPoints )
	{
		model::Log::Error("EnemySpawner: spawner %c has nothing to send, skipped", CurrentSpawner);
		Queue.PopBack();
		return;
	}

	// Try the spawner's points in random order and take the first one that is free.
	TArray<cocos2d::Vec2> CurrentPoints = *SpawnerPoints;
	Shuffle(CurrentPoints);
	for ( const auto& Point : CurrentPoints )
	{
		if ( IsReserved(Point) || !IsFree(Point) )
		{
			continue;
		}

		const std::string EnemyId = Pool[model::GameRandom().Int(0, static_cast<int>(Pool.Size()) - 1)];
		Queue.PopBack();
		Timer = 0.0f;
		++Pending;
		Reserved.PushBack(Point);
		effects::SpawnFlash(World, Point);
		World->runAction(cocos2d::Sequence::create(
			cocos2d::DelayTime::create(FlashDuration),
			cocos2d::CallFunc::create([this, Point, EnemyId, CurrentSpawner]() { FinishSpawn(Point, EnemyId, CurrentSpawner); }),
			nullptr));
		return;
	}
}

bool EnemySpawner::IsReserved(const cocos2d::Vec2& InPoint) const
{
	return std::find(Reserved.begin(), Reserved.end(), InPoint) != Reserved.end();
}

void EnemySpawner::FinishSpawn(const cocos2d::Vec2& InPoint, std::string_view InEnemyId, const char InSpawner)
{
	--Pending;
	Reserved.Erase(std::remove(Reserved.begin(), Reserved.end(), InPoint), Reserved.end());

	// Something drove onto the point during the flash: this enemy comes later instead.
	if ( !IsFree(InPoint) )
	{
		Queue.PushBack(InSpawner);
		return;
	}
	SpawnAt(InPoint, InEnemyId);
}

const TArray<std::string>& EnemySpawner::GetPool(const char InSpawner)
{
	TArray<std::string>* const Found = Pools.Find(InSpawner);
	if ( Found )
	{
		return *Found;
	}
	return Pools.InsertOrAssign(InSpawner, LevelLoader::ResolvePool(LevelLoader::GetRule(Level, InSpawner)));
}

int EnemySpawner::CountAliveEnemies() const
{
	int CurrentCount = 0;
	GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&CurrentCount](Tank& InObject)
	{
		if ( InObject.GetTeam() == ETeam::Enemy && !InObject.IsDestroying() )
		{
			++CurrentCount;
		}
	});
	return CurrentCount;
}

bool EnemySpawner::IsFree(const cocos2d::Vec2& InPoint) const
{
	const float Side = TileSize - 2 * FreeAreaMargin;
	const cocos2d::Rect Area(InPoint.x - Side / 2, InPoint.y - Side / 2, Side, Side);
	return !GameContext::Get<ObjectsFactory>().FindObjectIf<PhysicsObject>([&Area](PhysicsObject& InObject)
	{
		return !InObject.IsPassable() && InObject.GetVisualNode()
			&& Area.intersectsRect(InObject.GetCollisionRect());
	});
}

void EnemySpawner::SpawnAt(const cocos2d::Vec2& InPoint, std::string_view InEnemyId)
{
	const EnemyDef* Enemy = GameContext::Get<EnemyCatalog>().Find(InEnemyId);
	if ( !Enemy )
	{
		return;
	}

	model::JsonValue::Object Overrides;
	Overrides.InsertOrAssign("position", model::JsonValue(model::Vec2f{ InPoint.x, InPoint.y }));

	auto CurrentTank = GameContext::Get<ObjectsFactory>().CreateFromConfig<Tank>(Enemy->ConfigId, World, Overrides);
	if ( !CurrentTank )
	{
		return;
	}

	++Spawned;
	CurrentTank->SetBounds(Bounds);
	CurrentTank->TurnTo(DownDegrees); // enemies come from the top
	if ( OnSpawned )
	{
		OnSpawned(*CurrentTank);
	}
}

} // namespace game

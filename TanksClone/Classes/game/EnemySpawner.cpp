#include "game/EnemySpawner.h"

#include <algorithm>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/Effects.h"
#include "game/EnemyCatalog.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "model/Random.h"

namespace tanks
{

namespace
{
	constexpr float FlashDuration = 0.5f;
	constexpr float DownDegrees = 180.0f;
	// The check area is a bit smaller than the tile: walls that only touch the tile
	// (even with a corner) must not block the spawn point.
	constexpr float FreeAreaMargin = 6.0f;

	template <typename T>
	void Shuffle(std::vector<T>& InItems)
	{
		for ( int Index = static_cast<int>(InItems.size()) - 1; Index > 0; --Index )
		{
			std::swap(InItems[Index], InItems[model::GameRandom().Int(0, Index)]);
		}
	}
}

EnemySpawner::EnemySpawner(cocos2d::Node* const InWorld, const cocos2d::Rect& InBounds, const LevelData& InLevel,
	const std::map<char, std::vector<cocos2d::Vec2>>& InPoints)
	: World(InWorld)
	, Bounds(InBounds)
	, Level(InLevel)
	, Points(InPoints)
{
	if ( Level.Waves.empty() )
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

int EnemySpawner::GetWaveCount() const
{
	return static_cast<int>(Level.Waves.size());
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
			if ( Queue.empty() && Pending == 0 )
			{
				CurrentState = State::WaitClear;
			}
			else if ( Timer >= CurrentWave.Interval && !Queue.empty() && CountAliveEnemies() + Pending < CurrentWave.MaxAlive )
			{
				TrySpawnNext();
			}
			break;

		case State::WaitClear:
			if ( CountAliveEnemies() == 0 && Pending == 0 )
			{
				++WaveIndex;
				Timer = 0.0f;
				CurrentState = WaveIndex < Level.Waves.size() ? State::Delay : State::Done;
			}
			break;

		default:
			break;
	}
}

void EnemySpawner::StartWave()
{
	const Wave& CurrentWave = Level.Waves[WaveIndex];

	Queue.clear();
	for ( const auto& CurrentEntry : CurrentWave.Entries )
	{
		Queue.insert(Queue.end(), std::max(0, CurrentEntry.Count), CurrentEntry.Spawner);
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
	const char CurrentSpawner = Queue.back();

	const auto& Pool = GetPool(CurrentSpawner);
	auto PointsIt = Points.find(CurrentSpawner);
	if ( Pool.empty() || PointsIt == Points.end() )
	{
		CCLOGERROR("EnemySpawner: spawner %c has nothing to send, skipped", CurrentSpawner);
		Queue.pop_back();
		return;
	}

	// Try the spawner's points in random order and take the first one that is free.
	std::vector<cocos2d::Vec2> CurrentPoints = PointsIt->second;
	Shuffle(CurrentPoints);
	for ( const auto& Point : CurrentPoints )
	{
		if ( IsReserved(Point) || !IsFree(Point) )
		{
			continue;
		}

		const std::string EnemyId = Pool[model::GameRandom().Int(0, static_cast<int>(Pool.size()) - 1)];
		Queue.pop_back();
		Timer = 0.0f;
		++Pending;
		Reserved.push_back(Point);
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
	Reserved.erase(std::remove(Reserved.begin(), Reserved.end(), InPoint), Reserved.end());

	// Something drove onto the point during the flash: this enemy comes later instead.
	if ( !IsFree(InPoint) )
	{
		Queue.push_back(InSpawner);
		return;
	}
	SpawnAt(InPoint, InEnemyId);
}

const std::vector<std::string>& EnemySpawner::GetPool(const char InSpawner)
{
	auto PoolIt = Pools.find(InSpawner);
	if ( PoolIt == Pools.end() )
	{
		PoolIt = Pools.insert({ InSpawner, LevelLoader::ResolvePool(LevelLoader::GetRule(Level, InSpawner)) }).first;
	}
	return PoolIt->second;
}

int EnemySpawner::CountAliveEnemies() const
{
	int CurrentCount = 0;
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject(ObjectType::Vehicle, [&CurrentCount](GameObject& InObject)
	{
		if ( InObject.GetTeam() == Team::Enemy && !InObject.IsDestroying() )
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
	return !GameContext::Get().GetManager<ObjectsFactory>().FindObjectIf([&Area](GameObject& InObject)
	{
		auto Physics = ObjectCast<PhysicsObject>(&InObject);
		return Physics && !Physics->IsPassable() && Physics->GetVisualNode()
			&& Area.intersectsRect(Physics->GetCollisionRect());
	});
}

void EnemySpawner::SpawnAt(const cocos2d::Vec2& InPoint, std::string_view InEnemyId)
{
	const EnemyDef* Enemy = GameContext::Get().GetManager<EnemyCatalog>().Find(InEnemyId);
	if ( !Enemy )
	{
		return;
	}

	cocos2d::ValueVector Position;
	Position.push_back(cocos2d::Value(InPoint.x));
	Position.push_back(cocos2d::Value(InPoint.y));
	cocos2d::ValueMap Overrides;
	Overrides["position"] = Position;

	auto CurrentTank = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(Enemy->ConfigId, World, Overrides);
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

} // namespace tanks

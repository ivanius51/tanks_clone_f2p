#include "game/SceneScript.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/GameCommand.h"
#include "game/ObjectsFactory.h"
#include "game/ai/TankAi.h"
#include "game/components/Buff.h"
#include "game/objects/Tank.h"
#include "model/AiTactic.h"
#include "model/GameEvents.h"

namespace game
{

namespace
{
	constexpr float ArriveShare = 0.1f;   // of a tile
	constexpr float FacingDot = 0.9f;
	const std::string EnemySlotPrefix = "enemy#";
	const std::string DestroyedEvent = ".destroyed";

}

SceneScript::SceneScript(const model::Script& InScript, const int InMapRows, TFunction<void(const std::string&)> InShowCaption)
	: MapRows(InMapRows)
	, Player(InScript)
	, ShowCaptionText(std::move(InShowCaption))
{
	for ( const auto& [Actor, Slot] : InScript.Actors )
	{
		SlotToActor[Slot] = Actor;
	}
}

void SceneScript::OnEnemySpawned(Tank& InEnemy)
{
	++EnemiesSpawned;
	// A destroyed actor's memory can be reused by the new tank: drop any actor bound to that address.
	for ( auto It = Actors.begin(); It != Actors.end(); )
	{
		It = It->second == &InEnemy ? Actors.erase(It) : std::next(It);
	}
	const auto Found = SlotToActor.find(EnemySlotPrefix + std::to_string(EnemiesSpawned));
	if ( Found == SlotToActor.end() || !IsRunning() )
	{
		return;
	}
	Actors[Found->second] = &InEnemy;
	Bound[Found->second] = true;
	InEnemy.SetAiSuspended(true);
	InEnemy.SetFriendlyFire(true);
}

void SceneScript::Update(const float InDelta)
{
	if ( Player.IsRunning() )
	{
		Player.Update(InDelta, *this);
	}
	if ( !Player.IsRunning() && !bReleased )
	{
		ReleaseActors();
	}
}

bool SceneScript::IsRunning() const
{
	return Player.IsRunning();
}

Tank* SceneScript::FindActor(const std::string& InActor) const
{
	const auto Found = Actors.find(InActor);
	if ( Found == Actors.end() )
	{
		return nullptr;
	}
	// The pointer is used only while the factory still holds that tank alive.
	Tank* Alive = nullptr;
	GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
	{
		if ( &InTank == Found->second && !InTank.IsDestroying() && InTank.GetVisualNode() )
		{
			Alive = &InTank;
		}
	});
	return Alive;
}

bool SceneScript::HasActor(const std::string& InActor) const
{
	return FindActor(InActor) != nullptr;
}

bool SceneScript::IsAt(const std::string& InActor, const model::Vec2f& InCell) const
{
	const Tank* const Actor = FindActor(InActor);
	return Actor && Actor->GetVisualNode()->getPosition().distance(cocos2d::Vec2((InCell.X + 0.5f) * TileSize, CellToWorldY(InCell.Y + 0.5f))) <= TileSize * ArriveShare;
}

bool SceneScript::IsFacing(const std::string& InActor, const std::string& InTarget) const
{
	const Tank* const Actor = FindActor(InActor);
	const Tank* const Target = FindActor(InTarget);
	if ( !Actor || !Target )
	{
		return false;
	}
	const cocos2d::Vec2 Towards = (Target->GetVisualNode()->getPosition() - Actor->GetVisualNode()->getPosition()).getNormalized();
	return Actor->GetDirection().dot(Towards) >= FacingDot && !Actor->IsTurning();
}

bool SceneScript::IsGone(const std::string& InActor) const
{
	return Bound.count(InActor) > 0 && !HasActor(InActor);
}

float SceneScript::CellToWorldY(const float InRow) const
{
	return (static_cast<float>(MapRows) - InRow) * TileSize;
}

float SceneScript::WorldYToRow(const float InWorldY) const
{
	return static_cast<float>(MapRows) - InWorldY / TileSize;
}

model::Vec2f SceneScript::GetCell(const std::string& InActor) const
{
	const Tank* const Actor = FindActor(InActor);
	if ( !Actor )
	{
		return model::Vec2f();
	}
	const cocos2d::Vec2 Position = Actor->GetVisualNode()->getPosition();
	return model::Vec2f(std::floor(Position.x / TileSize), std::floor(WorldYToRow(Position.y)));
}

bool SceneScript::HasEvent(const std::string& InEvent) const
{
	if ( InEvent.size() > DestroyedEvent.size() && InEvent.compare(InEvent.size() - DestroyedEvent.size(), DestroyedEvent.size(), DestroyedEvent) == 0 )
	{
		const std::string Actor = InEvent.substr(0, InEvent.size() - DestroyedEvent.size());
		return IsGone(Actor);
	}
	return GameContext::Get<model::GameEvents>().Has(InEvent);
}

void SceneScript::SteerTowards(Tank& InActor, const cocos2d::Vec2& InPoint, const bool bInMove)
{
	const cocos2d::Vec2 Offset = InPoint - InActor.GetVisualNode()->getPosition();
	// A move goes down or up first, then sideways: level rows near the spawns are kept open for it.
	const bool bAlongX = bInMove ? std::abs(Offset.y) <= TileSize * ArriveShare : std::abs(Offset.x) >= std::abs(Offset.y);
	CommandType Command = CommandType::Undefined;
	if ( bAlongX )
	{
		Command = Offset.x > 0.0f ? CommandType::MoveRight : CommandType::MoveLeft;
	}
	else
	{
		Command = Offset.y > 0.0f ? CommandType::MoveForward : CommandType::MoveBack;
	}
	const cocos2d::Vec2 Wanted = bAlongX ? cocos2d::Vec2(Offset.x > 0.0f ? 1.0f : -1.0f, 0.0f) : cocos2d::Vec2(0.0f, Offset.y > 0.0f ? 1.0f : -1.0f);
	if ( !bInMove && InActor.GetDirection().dot(Wanted) >= FacingDot )
	{
		return;
	}
	if ( !InActor.IsRunning() || InActor.GetDirection().dot(Wanted) < FacingDot )
	{
		InActor.OnCommandReceived(GameCommand(Command));
	}
}

void SceneScript::MoveTo(const std::string& InActor, const model::Vec2f& InCell)
{
	if ( Tank* const Actor = FindActor(InActor) )
	{
		SteerTowards(*Actor, cocos2d::Vec2((InCell.X + 0.5f) * TileSize, CellToWorldY(InCell.Y + 0.5f)), true);
	}
}

void SceneScript::Face(const std::string& InActor, const std::string& InTarget)
{
	Tank* const Actor = FindActor(InActor);
	const Tank* const Target = FindActor(InTarget);
	if ( Actor && Target )
	{
		SteerTowards(*Actor, Target->GetVisualNode()->getPosition(), false);
	}
}

void SceneScript::Shoot(const std::string& InActor)
{
	if ( Tank* const Actor = FindActor(InActor) )
	{
		Actor->OnCommandReceived(GameCommand(CommandType::Shoot));
	}
}

void SceneScript::ShowCaption(const std::string& InText)
{
	if ( ShowCaptionText )
	{
		ShowCaptionText(InText);
	}
}

void SceneScript::StopActor(const std::string& InActor)
{
	if ( Tank* const Actor = FindActor(InActor) )
	{
		Actor->OnCommandReceived(GameCommand(CommandType::Stop));
	}
}

void SceneScript::SetHealth(const std::string& InActor, const float InShare)
{
	if ( Tank* const Actor = FindActor(InActor) )
	{
		Actor->SetHealth(std::max(1, static_cast<int>(std::round(Actor->GetMaxHealth() * InShare))));
	}
}

void SceneScript::ApplyBuff(const std::string& InActor, const std::string& InBuff)
{
	Tank* const Actor = FindActor(InActor);
	const BuffDef* const Found = GameContext::Get<BuffCatalog>().Find(InBuff);
	if ( Actor && Found )
	{
		BuffDef Permanent = *Found;
		Permanent.Duration = -1.0f;
		Actor->GetBuffs().Add(Permanent);
	}
}

void SceneScript::SetTactic(const std::string& InActor, const std::string& InTactic)
{
	Tank* const Actor = FindActor(InActor);
	if ( !Actor || !Actor->GetAi() )
	{
		return;
	}
	for ( std::size_t Index = 0; Index < static_cast<std::size_t>(model::AiTactic::Count); ++Index )
	{
		if ( model::GetTacticName(static_cast<model::AiTactic>(Index)) == InTactic )
		{
			Actor->GetAi()->ForceTactic(static_cast<model::AiTactic>(Index));
			return;
		}
	}
}

void SceneScript::EndLevel(const bool bInWin)
{
	if ( EndLevelHandler )
	{
		EndLevelHandler(bInWin);
	}
}

int SceneScript::GetCounter(const std::string& InName) const
{
	return CounterSource ? CounterSource(InName) : 0;
}

bool SceneScript::IsPlayerInArea(const model::Vec2f& InOrigin, const model::Vec2f& InSize) const
{
	const Tank* const PlayerTank = PlayerSource ? PlayerSource() : nullptr;
	if ( !PlayerTank || !PlayerTank->GetVisualNode() )
	{
		return false;
	}
	const cocos2d::Vec2 Position = PlayerTank->GetVisualNode()->getPosition();
	const float Column = Position.x / TileSize;
	const float Row = WorldYToRow(Position.y);
	return Column >= InOrigin.X && Column < InOrigin.X + InSize.X && Row >= InOrigin.Y && Row < InOrigin.Y + InSize.Y;
}

void SceneScript::ReleaseActors()
{
	bReleased = true;
	for ( const auto& Entry : Actors )
	{
		if ( Tank* const Actor = FindActor(Entry.first) )
		{
			Actor->SetAiSuspended(false);
		}
	}
}

} // namespace game

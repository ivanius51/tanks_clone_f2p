#include "game/ai/TankAi.h"

#include <algorithm>
#include <cmath>

#include "core/GameConstants.h"
#include "game/ai/AiQueries.h"
#include "game/ai/AiSettingsLoader.h"
#include "game/objects/Tank.h"
#include "model/Random.h"

namespace tanks
{

namespace
{
	// The facing is a unit axis vector: a component above this means the tank faces along that axis.
	constexpr float AxisThreshold = 0.5f;

	bool IsVerticalMove(const CommandType InCommand)
	{
		return InCommand == CommandType::MoveForward || InCommand == CommandType::MoveBack;
	}

	bool IsHorizontalMove(const CommandType InCommand)
	{
		return InCommand == CommandType::MoveLeft || InCommand == CommandType::MoveRight;
	}

	bool RollChance(const float InChance)
	{
		return model::GameRandom().Chance(InChance);
	}
}

TankAi::TankAi(Tank& InOwner, const AiType InType)
	: Owner(InOwner)
	, Type(InType)
	, Settings(AiSettingsLoader::LoadTankAi(InType))
{
}

void TankAi::Update(const float InDelta)
{
	ThinkTimer -= InDelta;
	if ( ThinkTimer <= 0.0f )
	{
		ChooseDirection();
		ThinkTimer = model::GameRandom().Float(Settings.GetThinkIntervalMin(), Settings.GetThinkIntervalMax());
	}
	ApplyPendingCommand(InDelta);
	TryToShoot(InDelta);

	if ( RollChance(Settings.GetSecondaryAbilityRate() * InDelta) )
	{
		Owner.UseSecondaryAbility();
	}
}

void TankAi::OnBlocked()
{
	ThinkTimer = 0.0f;
}

AiType TankAi::GetType() const
{
	return Type;
}

const TankAiSettings& TankAi::GetSettings() const
{
	return Settings;
}

void TankAi::SetSettings(const TankAiSettings& InSettings)
{
	Settings = InSettings;
}

void TankAi::ScaleBehaviour(const float InThinkScale, const float InFireRateScale, const float InAimScale)
{
	const float Think = std::max(0.05f, InThinkScale);
	Settings.SetThinkInterval(Settings.GetThinkIntervalMin() * Think, Settings.GetThinkIntervalMax() * Think);
	Settings.SetRandomFireRate(Settings.GetRandomFireRate() * std::max(0.0f, InFireRateScale));
	Settings.SetAimTolerance(Settings.GetAimTolerance() * std::max(0.0f, InAimScale));
}

CommandType TankAi::GetLastCommand() const
{
	return LastCommand;
}

CommandType TankAi::GetPendingCommand() const
{
	return PendingCommand;
}

void TankAi::ChooseDirection()
{
	CommandType Command = AiQueries::RandomMove();

	const Tank* Player = AiQueries::FindPlayer();
	const bool bCanFocus = Settings.GetFocusOnPlayerChance() > 0.0f && Player && Player->GetVisualNode() && Owner.GetVisualNode();
	if ( bCanFocus && RollChance(Settings.GetFocusOnPlayerChance()) )
	{
		Command = AiQueries::MoveAlongMajorAxis(Player->GetVisualNode()->getPosition() - Owner.GetVisualNode()->getPosition());
	}

	// After being blocked do not push into the same wall again.
	if ( Command == LastCommand && Owner.IsBlocked() )
	{
		Command = AiQueries::RandomMove();
	}

	LastCommand = Command;
	PendingCommand = Command;
	PendingTime = 0.0f;
}

bool TankAi::IsMovingVertically() const
{
	return std::fabs(Owner.GetDirection().y) > AxisThreshold;
}

bool TankAi::IsNearCellCenter(const float InDelta) const
{
	const cocos2d::Node* OwnerNode = Owner.GetVisualNode();
	if ( !OwnerNode || !Owner.HasBounds() )
	{
		return true;
	}
	const bool bVertical = IsMovingVertically();
	const float Coordinate = bVertical ? OwnerNode->getPositionY() : OwnerNode->getPositionX();
	const float Origin = bVertical ? Owner.GetBounds().getMinY() : Owner.GetBounds().getMinX();
	const float Offset = std::fmod(Coordinate - Origin, TileSize) - TileSize / 2;
	return std::fabs(Offset) <= std::max(Settings.GetCellCenterTolerance(), Owner.GetSpeed() * InDelta);
}

void TankAi::ApplyPendingCommand(const float InDelta)
{
	if ( PendingCommand == CommandType::Undefined )
	{
		return;
	}
	PendingTime += InDelta;

	const bool bMovingVertically = IsMovingVertically();
	const bool bTurnsAcross = Owner.IsRunning()
		&& ((IsVerticalMove(PendingCommand) && !bMovingVertically) || (IsHorizontalMove(PendingCommand) && bMovingVertically));
	if ( bTurnsAcross && !Owner.IsBlocked() && PendingTime < Settings.GetMaxTurnWait() && !IsNearCellCenter(InDelta) )
	{
		return;
	}

	Owner.OnCommandReceived(GameCommand(PendingCommand));
	PendingCommand = CommandType::Undefined;
}

void TankAi::TryToShoot(const float InDelta)
{
	const cocos2d::Node* OwnerNode = Owner.GetVisualNode();
	if ( !OwnerNode )
	{
		return;
	}

	bool bShoot = RollChance(Settings.GetRandomFireRate() * InDelta);

	const Tank* Player = AiQueries::FindPlayer();
	if ( Player && Player->GetVisualNode() )
	{
		const cocos2d::Vec2 Delta = Player->GetVisualNode()->getPosition() - OwnerNode->getPosition();
		const cocos2d::Vec2& Direction = Owner.GetDirection();
		const bool bInLineOfFire = Delta.dot(Direction) > 0.0f && std::fabs(Delta.cross(Direction)) < Settings.GetAimTolerance();
		bShoot = bShoot || bInLineOfFire;
	}

	if ( bShoot )
	{
		Owner.Shoot();
	}
}

} // namespace tanks

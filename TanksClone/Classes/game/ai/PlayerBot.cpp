#include "game/ai/PlayerBot.h"

#include <cmath>

#include "game/ai/AiQueries.h"
#include "game/ai/AiSettingsLoader.h"
#include "game/objects/Tank.h"

namespace game
{

namespace
{
	Tank* FindDrivablePlayer()
	{
		Tank* Player = AiQueries::FindPlayer();
		return Player && Player->GetVisualNode() ? Player : nullptr;
	}

	cocos2d::Vec2 PositionOf(const Tank& InTank)
	{
		return InTank.GetVisualNode()->getPosition();
	}

	cocos2d::Vec2 AxisTowards(const cocos2d::Vec2& InDelta, const bool bInVertical)
	{
		return bInVertical ? cocos2d::Vec2(0.0f, InDelta.y > 0.0f ? 1.0f : -1.0f) : cocos2d::Vec2(InDelta.x > 0.0f ? 1.0f : -1.0f, 0.0f);
	}
}

PlayerBot::PlayerBot()
	: Settings(AiSettingsLoader::LoadPlayerBot())
{
}

void PlayerBot::Update(const float InDelta)
{
	Tank* Player = FindDrivablePlayer();
	if ( !Player )
	{
		return;
	}

	ChooseWeapon(*Player);

	BlockedTimer = Player->IsBlocked() ? BlockedTimer + InDelta : 0.0f;
	ThinkTimer -= InDelta;
	if ( ThinkTimer <= 0.0f || BlockedTimer > Settings.GetStuckTime() )
	{
		Think();
		ThinkTimer = Settings.GetThinkInterval();
		BlockedTimer = 0.0f;
	}

	// Something in the way (usually a brick wall): shoot it.
	if ( Player->IsBlocked() )
	{
		Player->Shoot();
	}

	ShootWhenLinedUp(*Player);
}

void PlayerBot::ShootWhenLinedUp(Tank& InPlayer)
{
	const Tank* Enemy = AiQueries::FindNearestEnemy(PositionOf(InPlayer));
	if ( !Enemy )
	{
		return;
	}
	const cocos2d::Vec2 Delta = PositionOf(*Enemy) - PositionOf(InPlayer);
	const cocos2d::Vec2& Direction = InPlayer.GetDirection();
	if ( Delta.dot(Direction) > 0.0f && std::fabs(Delta.cross(Direction)) < Settings.GetAimTolerance() )
	{
		InPlayer.Shoot();
	}
	if ( Delta.length() < Settings.GetAbilityRange() )
	{
		InPlayer.UseSecondaryAbility();
	}
}

void PlayerBot::ChooseWeapon(Tank& InPlayer)
{
	const bool bUsesAmmo = InPlayer.GetActiveWeapon().bUsesAmmo;
	const bool bOutOfShells = bUsesAmmo && !InPlayer.CanFireActiveWeapon();
	const bool bShellsBack = !bUsesAmmo && InPlayer.GetAmmo() > 0;
	if ( bOutOfShells || bShellsBack )
	{
		InPlayer.SwitchWeapon();
	}
	if ( !InPlayer.GetActiveWeapon().bUsesAmmo )
	{
		bUsedFreeWeapon = true;
	}
}

bool PlayerBot::HasUsedFreeWeapon() const
{
	return bUsedFreeWeapon;
}

ECommandType PlayerBot::GetLastMove() const
{
	return LastMove;
}

const PlayerBotSettings& PlayerBot::GetSettings() const
{
	return Settings;
}

void PlayerBot::SetSettings(const PlayerBotSettings& InSettings)
{
	Settings = InSettings;
}

void PlayerBot::Think()
{
	Tank* Player = FindDrivablePlayer();
	if ( !Player )
	{
		return;
	}

	const Tank* Enemy = AiQueries::FindNearestEnemy(PositionOf(*Player));
	const ECommandType Move = Enemy
		? ChooseMoveTowards(*Player, PositionOf(*Enemy) - PositionOf(*Player))
		: ChooseMoveWithoutEnemies(*Player);
	GiveMove(*Player, Move);
}

ECommandType PlayerBot::ChooseMoveWithoutEnemies(const Tank& InPlayer) const
{
	// Back off towards the home row, so the bot never sits on a spawn point (an occupied spawn point sends nobody).
	const bool bAtHome = !InPlayer.HasBounds() || PositionOf(InPlayer).y < InPlayer.GetBounds().getMinY() + Settings.GetHomeHeight();
	return bAtHome || InPlayer.IsBlocked() ? ECommandType::Stop : ECommandType::MoveBack;
}

ECommandType PlayerBot::ChooseMoveTowards(const Tank& InPlayer, const cocos2d::Vec2& InDelta) const
{
	if ( InPlayer.IsBlocked() )
	{
		return AiQueries::RandomMove();
	}

	const float AimTolerance = Settings.GetAimTolerance();
	const bool bLinedUpVertically = std::fabs(InDelta.x) < AimTolerance;
	const bool bLinedUpHorizontally = std::fabs(InDelta.y) < AimTolerance;
	if ( !bLinedUpVertically && !bLinedUpHorizontally )
	{
		return AiQueries::MoveAlongMinorAxis(InDelta);
	}

	const cocos2d::Vec2 Wanted = AxisTowards(InDelta, bLinedUpVertically);
	const bool bFacesEnemy = InPlayer.GetDirection().dot(Wanted) > Settings.GetFacingAlignment();
	if ( bFacesEnemy && InDelta.length() < Settings.GetHoldDistance() )
	{
		return ECommandType::Stop;
	}
	return bLinedUpVertically
		? (InDelta.y > 0.0f ? ECommandType::MoveForward : ECommandType::MoveBack)
		: (InDelta.x > 0.0f ? ECommandType::MoveRight : ECommandType::MoveLeft);
}

void PlayerBot::GiveMove(Tank& InPlayer, const ECommandType InMove)
{
	LastMove = InMove;
	InPlayer.OnCommandReceived(GameCommand(InMove));
}

} // namespace game

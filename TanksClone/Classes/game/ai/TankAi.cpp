#include "game/ai/TankAi.h"

#include <algorithm>
#include <cmath>

#include "core/GameConstants.h"
#include "game/ai/AiQueries.h"
#include "game/ai/AiSettingsLoader.h"
#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Bullet.h"
#include "game/objects/Tank.h"
#include "model/AiTactics.h"
#include "ui/CocosConvert.h"
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
	TryToDodge(InDelta);
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
	if ( bCanFocus && model::AiTactics::ShouldRetreat(GetHealthShare(), Settings.GetRetreatHealth()) )
	{
		Command = AiQueries::MoveAlongMajorAxis(Owner.GetVisualNode()->getPosition() - Player->GetVisualNode()->getPosition());
	}
	else if ( bCanFocus && RollChance(Settings.GetFocusOnPlayerChance()) )
	{
		const cocos2d::Vec2 ToPlayer = Player->GetVisualNode()->getPosition() - Owner.GetVisualNode()->getPosition();
		Command = RollChance(Settings.GetFlankChance()) ? AiQueries::MoveAlongMinorAxis(ToPlayer) : AiQueries::MoveAlongMajorAxis(ToPlayer);
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

float TankAi::GetHealthShare() const
{
	return Owner.GetMaxHealth() > 0 ? static_cast<float>(Owner.GetHealth()) / Owner.GetMaxHealth() : 1.0f;
}

cocos2d::Vec2 TankAi::AimPoint(const Tank& InPlayer) const
{
	const cocos2d::Vec2 Target = InPlayer.GetVisualNode()->getPosition();
	if ( !Settings.ShouldLeadShots() || !InPlayer.IsRunning() || Owner.GetActiveWeapon().BulletConfig.empty() )
	{
		return Target;
	}
	const model::IConfigNode& BulletNode = GameContext::Get().GetManager<ConfigReader>().GetConfig(Owner.GetActiveWeapon().BulletConfig);
	const float BulletSpeed = BulletNode.GetValueOr("speed", 0.0f) * Owner.GetBulletSpeedScale();
	const model::Vec2f Velocity = view::cocos::FromCocos(InPlayer.GetDirection()) * InPlayer.GetSpeed();
	return view::cocos::ToCocos(model::AiTactics::LeadPoint(view::cocos::FromCocos(Owner.GetVisualNode()->getPosition()),
		view::cocos::FromCocos(Target), Velocity, BulletSpeed));
}

void TankAi::TryToDodge(const float InDelta)
{
	DodgeTimer -= InDelta;
	const cocos2d::Node* OwnerNode = Owner.GetVisualNode();
	if ( Settings.GetDodgeChance() <= 0.0f || DodgeTimer > 0.0f || !OwnerNode )
	{
		return;
	}
	const model::Vec2f Position = view::cocos::FromCocos(OwnerNode->getPosition());
	const float Radius = Owner.GetCollisionRect().size.width * 0.5f + TileSize * 0.25f;
	const Bullet* Threat = nullptr;
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Bullet>([&](Bullet& InBullet)
	{
		if ( !Threat && !InBullet.IsDestroying() && InBullet.GetTeam() != Owner.GetTeam() && InBullet.GetVisualNode()
			&& model::AiTactics::IsThreat(Position, Radius, view::cocos::FromCocos(InBullet.GetVisualNode()->getPosition()),
				view::cocos::FromCocos(InBullet.GetVelocity()), Settings.GetDodgeLookahead()) )
		{
			Threat = &InBullet;
		}
	});
	if ( !Threat )
	{
		return;
	}
	DodgeTimer = Settings.GetDodgeLookahead();
	if ( !RollChance(Settings.GetDodgeChance()) )
	{
		return;
	}
	const model::Vec2f Away = model::AiTactics::DodgeDirection(Position, view::cocos::FromCocos(Threat->GetVisualNode()->getPosition()),
		view::cocos::FromCocos(Threat->GetVelocity()));
	PendingCommand = AiQueries::MoveAlongMajorAxis(view::cocos::ToCocos(Away));
	LastCommand = PendingCommand;
	PendingTime = Settings.GetMaxTurnWait();
	ThinkTimer = std::max(ThinkTimer, Settings.GetDodgeLookahead());
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
		const cocos2d::Vec2 Delta = AimPoint(*Player) - OwnerNode->getPosition();
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

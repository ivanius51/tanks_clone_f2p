#include "game/ai/TankAi.h"

#include <algorithm>
#include <cmath>

#include "core/GameConstants.h"
#include "game/ai/AiQueries.h"
#include "game/ai/AiSettingsLoader.h"
#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "game/ObjectsFactory.h"
#include "game/objects/Bonus.h"
#include "game/objects/Bullet.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "model/AiTactics.h"
#include "ui/CocosConvert.h"
#include "model/Random.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	// The facing is a unit axis vector: a component above this means the tank faces along that axis.
	constexpr float AxisThreshold = 0.5f;

	bool IsVerticalMove(const ECommandType InCommand)
	{
		return InCommand == ECommandType::MoveForward || InCommand == ECommandType::MoveBack;
	}

	bool IsHorizontalMove(const ECommandType InCommand)
	{
		return InCommand == ECommandType::MoveLeft || InCommand == ECommandType::MoveRight;
	}

	bool RollChance(const float InChance)
	{
		return model::GameRandom().Chance(InChance);
	}
}

TankAi::TankAi(Tank& InOwner, const EAiType InType)
	: Owner(InOwner)
	, Type(InType)
	, Settings(AiSettingsLoader::LoadTankAi(InType))
{
	Features.Apply(Settings.GetFeatures());
	Camp.Configure(Settings.GetCampRadius(), Settings.GetCampTime());
}

void TankAi::Update(const float InDelta)
{
	ThinkTimer -= InDelta;
	const Tank* const Player = AiQueries::FindPlayer();
	if ( HasFeature(model::AiFeature::AntiCamp) && Player && Player->GetVisualNode() )
	{
		Camp.Update(view::cocos::FromCocos(Player->GetVisualNode()->getPosition()), InDelta);
	}
	if ( ThinkTimer <= 0.0f )
	{
		ChooseDirection();
		ThinkTimer = model::GameRandom().Float(Settings.GetThinkIntervalMin(), Settings.GetThinkIntervalMax());
	}
	TryToDodge(InDelta);
	TryToTakeCover();
	UpdateFakeTurn(InDelta);
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

EAiType TankAi::GetType() const
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

ECommandType TankAi::GetLastCommand() const
{
	return LastCommand;
}

ECommandType TankAi::GetPendingCommand() const
{
	return PendingCommand;
}

void TankAi::ChooseDirection()
{
	ECommandType Command = AiQueries::RandomMove();
	Tactic = model::AiTactic::Wander;

	const Tank* Player = AiQueries::FindPlayer();
	const bool bCanFocus = Settings.GetFocusOnPlayerChance() > 0.0f && Player && Player->GetVisualNode() && Owner.GetVisualNode();
	const ECommandType GuardMove = HasFeature(model::AiFeature::GuardDrops) ? GuardDropMove() : ECommandType::Undefined;
	if ( bCanFocus && HasFeature(model::AiFeature::Retreat) && model::AiTactics::ShouldRetreat(GetHealthShare(), Settings.GetRetreatHealth()) )
	{
		Command = AiQueries::MoveAlongMajorAxis(Owner.GetVisualNode()->getPosition() - Player->GetVisualNode()->getPosition());
		Tactic = model::AiTactic::Retreat;
	}
	else if ( GuardMove != ECommandType::Undefined )
	{
		Command = GuardMove;
		Tactic = model::AiTactic::GuardDrop;
	}
	else if ( const Wall* const Base = HasFeature(model::AiFeature::AttackBase) && !IsPlayerCamping() && Owner.GetVisualNode()
		&& RollChance(Settings.GetBaseFocusChance()) ? AiQueries::FindBase() : nullptr )
	{
		Command = HeadFor(Base->GetVisualNode()->getPosition());
		Tactic = model::AiTactic::AttackBase;
	}
	else if ( bCanFocus && (IsPlayerCamping() || RollChance(Settings.GetFocusOnPlayerChance())) )
	{
		const cocos2d::Vec2 PlayerPosition = Player->GetVisualNode()->getPosition();
		const cocos2d::Vec2 ToPlayer = PlayerPosition - Owner.GetVisualNode()->getPosition();
		bool bFlank = HasFeature(model::AiFeature::Flank) && RollChance(Settings.GetFlankChance());
		Tactic = bFlank ? model::AiTactic::Flank : model::AiTactic::Chase;
		if ( HasFeature(model::AiFeature::Group) )
		{
			bFlank = !IsGroupPinner(PlayerPosition);
			Tactic = bFlank ? model::AiTactic::Flank : model::AiTactic::Pin;
		}
		if ( IsPlayerCamping() && !bFlank )
		{
			Tactic = model::AiTactic::HuntCamper;
		}
		Command = bFlank ? AiQueries::MoveAlongMinorAxis(ToPlayer) : HeadFor(PlayerPosition);
	}

	if ( bCanFocus && HasFeature(model::AiFeature::FakeTurn) && RollChance(Settings.GetFakeTurnChance()) )
	{
		Command = FakeTurn(Command);
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

ECommandType TankAi::HeadFor(const cocos2d::Vec2& InTarget) const
{
	const ECommandType Command = AiQueries::MoveAlongPath(Owner, InTarget, Settings.GetPathBreakCost());
	return Command != ECommandType::Undefined ? Command : AiQueries::MoveAlongMajorAxis(InTarget - Owner.GetVisualNode()->getPosition());
}

void TankAi::ApplyFeatures(const model::AiSwitches& InSwitches)
{
	Features.Apply(InSwitches);
}

bool TankAi::HasFeature(const model::AiFeature InFeature) const
{
	return Features.Has(InFeature);
}

model::AiTactic TankAi::GetTactic() const
{
	return Tactic;
}

void TankAi::ForceTactic(const model::AiTactic InTactic)
{
	Tactic = InTactic;
}

bool TankAi::IsPlayerCamping() const
{
	return HasFeature(model::AiFeature::AntiCamp) && Camp.IsCamping();
}

ECommandType TankAi::GuardDropMove() const
{
	const cocos2d::Vec2 Position = Owner.GetVisualNode()->getPosition();
	const Bonus* Nearest = nullptr;
	float NearestDistance = Settings.GetGuardRange();
	GameContext::Get<ObjectsFactory>().ForEachObject<Bonus>([&](Bonus& InPickup)
	{
		if ( !InPickup.IsDestroying() && InPickup.GetVisualNode() )
		{
			const float Distance = InPickup.GetVisualNode()->getPosition().distance(Position);
			if ( Distance < NearestDistance )
			{
				Nearest = &InPickup;
				NearestDistance = Distance;
			}
		}
	});
	if ( !Nearest || NearestDistance <= Settings.GetGuardRadius() )
	{
		return Nearest ? AiQueries::RandomMove() : ECommandType::Undefined;
	}
	return AiQueries::MoveAlongMajorAxis(Nearest->GetVisualNode()->getPosition() - Position);
}

bool TankAi::IsGroupPinner(const cocos2d::Vec2& InPlayerPosition) const
{
	const float Own = Owner.GetVisualNode()->getPosition().distance(InPlayerPosition);
	TArray<float> Others;
	GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InTank)
	{
		if ( &InTank != &Owner && !InTank.IsDestroying() && InTank.GetTeam() == Owner.GetTeam() && InTank.GetVisualNode()
			&& InTank.GetAi() && InTank.GetAi()->HasFeature(model::AiFeature::Group) )
		{
			Others.EmplaceBack(InTank.GetVisualNode()->getPosition().distance(InPlayerPosition));
		}
	});
	return model::AiTactics::IsClosest(Own, Others);
}

float TankAi::GetHealthShare() const
{
	return Owner.GetMaxHealth() > 0 ? static_cast<float>(Owner.GetHealth()) / Owner.GetMaxHealth() : 1.0f;
}

cocos2d::Vec2 TankAi::AimPoint(const Tank& InPlayer) const
{
	const cocos2d::Vec2 Target = InPlayer.GetVisualNode()->getPosition();
	if ( !HasFeature(model::AiFeature::LeadShots) || !InPlayer.IsRunning() || Owner.GetActiveWeapon().BulletConfig.IsEmpty() )
	{
		return Target;
	}
	const model::IConfigNode& BulletNode = GameContext::Get<ConfigReader>().GetConfig(Owner.GetActiveWeapon().BulletConfig);
	const float BulletSpeed = BulletNode.GetValueOr("speed", 0.0f) * Owner.GetBulletSpeedScale();
	const model::Vec2f Velocity = view::cocos::FromCocos(InPlayer.GetDirection()) * InPlayer.GetSpeed();
	return view::cocos::ToCocos(model::AiTactics::LeadPoint(view::cocos::FromCocos(Owner.GetVisualNode()->getPosition()),
		view::cocos::FromCocos(Target), Velocity, BulletSpeed));
}

void TankAi::TryToTakeCover()
{
	const Tank* const Player = AiQueries::FindPlayer();
	if ( !HasFeature(model::AiFeature::Cover) || DodgeTimer > 0.0f || Owner.GetReloadRemaining() <= 0.0f
		|| !Player || !Player->GetVisualNode() || !Owner.GetVisualNode() )
	{
		return;
	}
	const model::Vec2f Position = view::cocos::FromCocos(Owner.GetVisualNode()->getPosition());
	const model::Vec2f Shooter = view::cocos::FromCocos(Player->GetVisualNode()->getPosition());
	if ( !model::AiTactics::IsExposed(Position, Shooter, Settings.GetAimTolerance()) )
	{
		return;
	}
	ForceCommand(AiQueries::MoveAlongMajorAxis(view::cocos::ToCocos(model::AiTactics::SidestepDirection(Position, Shooter))));
	Tactic = model::AiTactic::Cover;
	DodgeTimer = Owner.GetReloadRemaining();
	ThinkTimer = std::max(ThinkTimer, DodgeTimer);
}

ECommandType TankAi::FakeTurn(const ECommandType InCommand)
{
	FakeTurnReturn = InCommand;
	FakeTurnTimer = Settings.GetFakeTurnTime();
	FakeTurnTactic = Tactic;
	Tactic = model::AiTactic::FakeTurn;
	const bool bFirst = RollChance(0.5f);
	if ( IsVerticalMove(InCommand) )
	{
		return bFirst ? ECommandType::MoveLeft : ECommandType::MoveRight;
	}
	return bFirst ? ECommandType::MoveForward : ECommandType::MoveBack;
}

void TankAi::UpdateFakeTurn(const float InDelta)
{
	if ( FakeTurnReturn == ECommandType::Undefined )
	{
		return;
	}
	FakeTurnTimer -= InDelta;
	if ( FakeTurnTimer <= 0.0f )
	{
		ForceCommand(FakeTurnReturn);
		FakeTurnReturn = ECommandType::Undefined;
		Tactic = FakeTurnTactic;
	}
}

void TankAi::ForceCommand(const ECommandType InCommand)
{
	PendingCommand = InCommand;
	LastCommand = InCommand;
	PendingTime = Settings.GetMaxTurnWait();
}

void TankAi::TryToDodge(const float InDelta)
{
	DodgeTimer -= InDelta;
	const cocos2d::Node* OwnerNode = Owner.GetVisualNode();
	if ( !HasFeature(model::AiFeature::Dodge) || Settings.GetDodgeChance() <= 0.0f || DodgeTimer > 0.0f || !OwnerNode )
	{
		return;
	}
	const model::Vec2f Position = view::cocos::FromCocos(OwnerNode->getPosition());
	const float Radius = Owner.GetCollisionRect().size.width * 0.5f + TileSize * 0.25f;
	const Bullet* Threat = nullptr;
	GameContext::Get<ObjectsFactory>().ForEachObject<Bullet>([&](Bullet& InBullet)
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
	ForceCommand(AiQueries::MoveAlongMajorAxis(view::cocos::ToCocos(Away)));
	Tactic = model::AiTactic::Dodge;
	FakeTurnReturn = ECommandType::Undefined;
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
	if ( PendingCommand == ECommandType::Undefined )
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
	PendingCommand = ECommandType::Undefined;
}

void TankAi::TryToShoot(const float InDelta)
{
	const cocos2d::Node* OwnerNode = Owner.GetVisualNode();
	if ( !OwnerNode )
	{
		return;
	}

	bool bShoot = RollChance(Settings.GetRandomFireRate() * InDelta);
	const cocos2d::Vec2& Direction = Owner.GetDirection();
	const auto IsInLineOfFire = [&](const cocos2d::Vec2& InTarget)
	{
		const cocos2d::Vec2 Delta = InTarget - OwnerNode->getPosition();
		return Delta.dot(Direction) > 0.0f && std::fabs(Delta.cross(Direction)) < Settings.GetAimTolerance();
	};

	const Tank* Player = AiQueries::FindPlayer();
	if ( Player && Player->GetVisualNode() )
	{
		bShoot = bShoot || IsInLineOfFire(AimPoint(*Player));
	}
	if ( HasFeature(model::AiFeature::AttackBase) )
	{
		const Wall* const Base = AiQueries::FindBase();
		bShoot = bShoot || (Base && IsInLineOfFire(Base->GetVisualNode()->getPosition()));
	}
	if ( !bShoot && (Owner.IsBlocked() || RollChance(Settings.GetClearWayRate() * InDelta)) )
	{
		bShoot = AiQueries::FindBreakableAhead(Owner, TileSize * Settings.GetClearWayTiles()) != nullptr;
	}

	if ( bShoot )
	{
		Owner.Shoot();
	}
}

} // namespace game

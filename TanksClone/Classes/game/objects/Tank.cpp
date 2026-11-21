#include "game/objects/Tank.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/Effects.h"
#include "game/ObjectsFactory.h"
#include "model/LobFlight.h"
#include "model/ShotSide.h"
#include "services/AudioManager.h"

namespace tanks
{

namespace
{
	constexpr float MuzzleDistance = TileSize * 0.6f;
	constexpr float RadiansToDegrees = 180.0f / 3.14159265358979323846f;
	constexpr int ShieldTag = 9001;
	constexpr float ShieldRadius = 25.0f; // in sprite pixels (the sprite is 32x32)
	constexpr float ShieldHitTime = 0.25f;
	constexpr float ShieldHitScale = 1.3f;
}

Tank::Tank() = default;
Tank::~Tank() = default;

std::unique_ptr<Tank> Tank::Create(cocos2d::Node* const InNode)
{
	std::unique_ptr<Tank> CurrentTank(new Tank());
	if ( !CurrentTank->Build() )
	{
		return nullptr;
	}
	CurrentTank->SetVisualNode(InNode);
	return CurrentTank;
}

bool Tank::Build()
{
	const bool bResult = MovableObject::Build();
	SetType(ObjectType::Vehicle);
	return bResult;
}

void Tank::ApplyDef(const TankDef& InDef)
{
	bUnlimitedAmmo = InDef.MaxAmmo < 0;
	Stats.SetBase(StatType::MaxHealth, static_cast<float>(GetMaxHealth()));
	Stats.SetBase(StatType::Damage, static_cast<float>(MovableObject::GetDamage()));
	Stats.SetBase(StatType::Armor, InDef.Armor);
	Stats.SetBase(StatType::Speed, GetSpeed());
	Stats.SetBase(StatType::ReloadTime, InDef.ReloadTime);
	Stats.SetBase(StatType::MaxAmmo, static_cast<float>(std::max(0, InDef.MaxAmmo)));
	Stats.SetBase(StatType::TurnSpeed, 1.0f);

	Weapons.clear();
	LockedWeapons.clear();
	for ( const WeaponDef& Weapon : InDef.Weapons )
	{
		(Weapon.bLocked ? LockedWeapons : Weapons).push_back(Weapon);
	}
	BurstLeft = 0;
	HitSound = InDef.HitSound;
	ArmorSound = InDef.ArmorSound;
	ExplodeSound = InDef.ExplodeSound;
	ReloadTimers.assign(Weapons.size(), 0.0f);
	ActiveWeapon = 0;

	Abilities.clear();
	AbilityDefs.clear();
	for ( const AbilityDef& CurrentDef : InDef.Abilities )
	{
		AddAbility(CurrentDef);
	}
	for ( const std::string& CurrentId : InDef.PermanentBuffs )
	{
		AddPermanentBuff(CurrentId);
	}
	Phases = InDef.Phases;
	std::vector<float> Thresholds;
	for ( const TankPhaseDef& Phase : Phases )
	{
		Thresholds.push_back(Phase.HealthBelow);
	}
	PhaseState.Configure(Thresholds);

	EnemyKind = InDef.EnemyKind;
	ScoreReward = InDef.ScoreReward;
	CoinReward = InDef.CoinReward;
	DropTable = InDef.DropTable;
	SplitInto = InDef.SplitInto;
	SplitCount = InDef.SplitInto.empty() ? 0 : InDef.SplitCount;
	SplitSpacingTiles = InDef.SplitSpacingTiles;
	bFrontalShield = InDef.bFrontalShield;
	RearDamageScale = InDef.RearDamageScale;
	Ai.reset(InDef.Ai != AiType::None ? new TankAi(*this, InDef.Ai) : nullptr);

	RefreshStats();
	RestoreFull();
}

void Tank::AddAbility(const AbilityDef& InDef)
{
	if ( auto CurrentAbility = CreateAbility(InDef.Type) )
	{
		AbilityDef Scaled = InDef;
		Scaled.Cooldown *= CooldownScale;
		CurrentAbility->Configure(Scaled);
		Abilities.push_back(std::move(CurrentAbility));
		AbilityDefs.push_back(InDef);
	}
}

void Tank::AddPermanentBuff(const std::string& InId)
{
	if ( const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(InId) )
	{
		BuffDef Permanent = *Buff;
		Permanent.Duration = -1.0f;
		Buffs.Add(Permanent);
	}
}

int Tank::GetPhase() const
{
	return PhaseState.GetCurrentPhase();
}

void Tank::SetOnPhaseCallback(const std::function<void(Tank&, const TankPhaseDef&)>& InCallback)
{
	OnPhaseCallback = InCallback;
}

void Tank::UpdatePhases()
{
	if ( Phases.empty() || IsDestroying() || GetMaxHealth() <= 0 )
	{
		return;
	}
	for ( const int Index : PhaseState.Update(static_cast<float>(GetHealth()) / GetMaxHealth()) )
	{
		StartPhase(Phases[Index]);
	}
}

void Tank::StartPhase(const TankPhaseDef& InPhase)
{
	for ( const AbilityDef& CurrentDef : InPhase.Abilities )
	{
		AddAbility(CurrentDef);
	}
	for ( const std::string& CurrentId : InPhase.Buffs )
	{
		AddPermanentBuff(CurrentId);
	}
	RefreshStats();
	GameContext::Get().GetManager<AudioManager>().Play(InPhase.Sound);
	if ( OnPhaseCallback )
	{
		OnPhaseCallback(*this, InPhase);
	}
}

const StatSet& Tank::GetStats() const
{
	return Stats;
}

BuffSet& Tank::GetBuffs()
{
	return Buffs;
}

const BuffSet& Tank::GetBuffs() const
{
	return Buffs;
}

void Tank::RefreshStats()
{
	Stats.ApplyModifiers(Buffs.CollectModifiers());

	SetSpeed(Stats.Get(StatType::Speed));
	SetTurnSpeed(Stats.Get(StatType::TurnSpeed));
	RescaleMaxHealth(Stats.GetInt(StatType::MaxHealth));
	if ( !bUnlimitedAmmo )
	{
		Ammo = std::min(Ammo, GetMaxAmmo());
	}
	UpdateShieldVisual();
}

// A translucent bubble around the tank while it is invulnerable.
void Tank::PlayShieldHit()
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode || !CurrentVisualNode->getChildByTag(ShieldTag) )
	{
		return;
	}
	const cocos2d::Vec2 Center(CurrentVisualNode->getContentSize().width / 2, CurrentVisualNode->getContentSize().height / 2);
	auto Ripple = cocos2d::DrawNode::create();
	Ripple->drawSolidCircle(Center, ShieldRadius, 0.0f, 36, cocos2d::Color4F(0.7f, 0.9f, 1.0f, 0.35f));
	Ripple->drawCircle(Center, ShieldRadius, 0.0f, 36, false, cocos2d::Color4F(1.0f, 1.0f, 1.0f, 1.0f));
	Ripple->setContentSize(CurrentVisualNode->getContentSize());
	Ripple->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	Ripple->setPosition(Center);
	Ripple->runAction(cocos2d::Sequence::create(
		cocos2d::Spawn::createWithTwoActions(cocos2d::ScaleTo::create(ShieldHitTime, ShieldHitScale), cocos2d::FadeOut::create(ShieldHitTime)),
		cocos2d::RemoveSelf::create(), nullptr));
	CurrentVisualNode->addChild(Ripple, 11);
}

void Tank::UpdateShieldVisual()
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode )
	{
		return;
	}

	cocos2d::Node* Bubble = CurrentVisualNode->getChildByTag(ShieldTag);
	if ( Buffs.IsInvulnerable() && !Bubble )
	{
		const cocos2d::Vec2 Center(CurrentVisualNode->getContentSize().width / 2, CurrentVisualNode->getContentSize().height / 2);
		auto Draw = cocos2d::DrawNode::create();
		Draw->drawSolidCircle(Center, ShieldRadius, 0.0f, 36, cocos2d::Color4F(0.35f, 0.65f, 1.0f, 0.25f));
		Draw->drawCircle(Center, ShieldRadius, 0.0f, 36, false, cocos2d::Color4F(0.65f, 0.88f, 1.0f, 0.95f));
		Draw->drawCircle(Center, ShieldRadius - 2.0f, 0.0f, 36, false, cocos2d::Color4F(0.5f, 0.8f, 1.0f, 0.5f));
		Draw->setTag(ShieldTag);
		Draw->setPosition(cocos2d::Vec2::ZERO);
		Draw->runAction(cocos2d::RepeatForever::create(cocos2d::Sequence::create(
			cocos2d::ScaleTo::create(0.45f, 1.07f), cocos2d::ScaleTo::create(0.45f, 1.0f), nullptr)));
		Draw->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
		CurrentVisualNode->addChild(Draw, 10);
	}
	else if ( !Buffs.IsInvulnerable() && Bubble )
	{
		Bubble->removeFromParent();
	}
}

void Tank::RestoreFull()
{
	Heal(GetMaxHealth());
	Ammo = bUnlimitedAmmo ? 0 : GetMaxAmmo();
}

size_t Tank::GetAbilityCount() const
{
	return Abilities.size();
}

const Ability* Tank::GetAbility(const size_t InIndex) const
{
	return InIndex < Abilities.size() ? Abilities[InIndex].get() : nullptr;
}

bool Tank::UseAbility(const size_t InIndex)
{
	return InIndex < Abilities.size() && !IsDestroying() && Abilities[InIndex]->Activate(*this);
}

bool Tank::UseSecondaryAbility()
{
	for ( size_t Index = 1; Index < Abilities.size(); ++Index )
	{
		if ( Abilities[Index]->GetReadiness() >= 1.0f && UseAbility(Index) )
		{
			return true;
		}
	}
	return false;
}

bool Tank::Shoot()
{
	return UseAbility(0);
}

void Tank::SetTriggerHeld(const bool bInHeld)
{
	bTriggerHeld = bInHeld;
}

bool Tank::IsTriggerHeld() const
{
	return bTriggerHeld;
}

bool Tank::EnableAutoFire(const std::string_view InWeaponId)
{
	bool bFound = false;
	for ( WeaponDef& Weapon : Weapons )
	{
		if ( Weapon.Id == InWeaponId )
		{
			Weapon.bAutoFire = true;
			bFound = true;
		}
	}
	return bFound;
}

bool Tank::CanAutoFire() const
{
	return !Weapons.empty() && GetActiveWeapon().bAutoFire;
}

bool Tank::Fire()
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode || !CurrentVisualNode->getParent() || IsDestroying() || Weapons.empty() || ReloadTimers[ActiveWeapon] > 0.0f
		|| !CanFireActiveWeapon() )
	{
		return false;
	}
	if ( IsTurning() )
	{
		bShotQueued = true;
		return false;
	}

	const WeaponDef& Weapon = GetActiveWeapon();
	if ( !SpawnVolley(Weapon) )
	{
		return false;
	}
	BurstLeft = std::max(0, Weapon.BurstCount - 1);
	BurstTimer = Weapon.BurstInterval;
	BurstWeapon = ActiveWeapon;

	if ( Weapon.bUsesAmmo && !bUnlimitedAmmo )
	{
		--Ammo;
	}
	ReloadTimers[ActiveWeapon] = GetActiveReloadTime();
	return true;
}

bool Tank::SpawnVolley(const WeaponDef& InWeapon)
{
	auto CurrentVisualNode = GetVisualNode();
	const cocos2d::Vec2& CurrentDirection = GetDirection();
	const float Side = InWeapon.bDropsBehind ? -1.0f : 1.0f;
	const cocos2d::Vec2 Muzzle = CurrentVisualNode->getPosition() + CurrentDirection * (MuzzleDistance * Side);
	const float Facing = std::atan2(CurrentDirection.x, CurrentDirection.y) * RadiansToDegrees;
	const int Pellets = std::max(1, InWeapon.Pellets);

	cocos2d::ValueVector Position;
	Position.push_back(cocos2d::Value(Muzzle.x));
	Position.push_back(cocos2d::Value(Muzzle.y));
	cocos2d::ValueMap Overrides;
	Overrides["position"] = Position;
	Overrides["damage"] = GetShotDamage(InWeapon);
	Overrides["speed_scale"] = BulletSpeedScale;
	Overrides["friendly_fire"] = bFriendlyFire;
	Overrides["team"] = GetTeam() == Team::Player ? "Player" : (GetTeam() == Team::Enemy ? "Enemy" : "Neutral");
	if ( InWeapon.bLobbed )
	{
		Overrides["lob_distance"] = model::LobFlight::ChooseDistance(FindFoeAhead() - MuzzleDistance,
			InWeapon.LobMinTiles * TileSize, InWeapon.LobMaxTiles * TileSize);
	}

	bool bSpawned = false;
	for ( int Index = 0; Index < Pellets; ++Index )
	{
		const float Offset = Pellets > 1 ? InWeapon.Spread * (static_cast<float>(Index) / (Pellets - 1) - 0.5f) : 0.0f;
		Overrides["rotation"] = Facing + Offset;
		auto CurrentBullet = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<PhysicsObject>(InWeapon.BulletConfig, CurrentVisualNode->getParent(), Overrides);
		if ( CurrentBullet && HasBounds() )
		{
			CurrentBullet->SetBounds(GetBounds());
		}
		bSpawned = bSpawned || CurrentBullet != nullptr;
	}
	if ( bSpawned )
	{
		GameContext::Get().GetManager<AudioManager>().Play(InWeapon.FireSound);
	}
	return bSpawned;
}

bool Tank::FireExtra(const WeaponDef& InWeapon)
{
	if ( IsDestroying() || !GetVisualNode() || !GetVisualNode()->getParent() )
	{
		return false;
	}
	return SpawnVolley(InWeapon);
}

float Tank::FindFoeAhead() const
{
	const cocos2d::Vec2 Origin = GetVisualNode()->getPosition();
	const cocos2d::Vec2& Ahead = GetDirection();
	float Nearest = 0.0f;
	GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
	{
		if ( &InOther == this || InOther.IsDestroying() || !InOther.GetVisualNode()
			|| InOther.GetTeam() == GetTeam() || InOther.GetTeam() == Team::Neutral )
		{
			return;
		}
		const cocos2d::Vec2 Offset = InOther.GetVisualNode()->getPosition() - Origin;
		const float Along = Offset.dot(Ahead);
		if ( Along > 0.0f && std::fabs(Offset.cross(Ahead)) <= TileSize && (Nearest <= 0.0f || Along < Nearest) )
		{
			Nearest = Along;
		}
	});
	return Nearest;
}

void Tank::UpdateBurst(const float InDelta)
{
	if ( BurstLeft <= 0 )
	{
		return;
	}
	BurstTimer -= InDelta;
	if ( BurstTimer > 0.0f || IsTurning() )
	{
		return;
	}
	if ( BurstWeapon != ActiveWeapon || IsDestroying() || !GetVisualNode() || !GetVisualNode()->getParent() )
	{
		BurstLeft = 0;
		return;
	}
	SpawnVolley(Weapons[BurstWeapon]);
	--BurstLeft;
	BurstTimer = Weapons[BurstWeapon].BurstInterval;
}

bool Tank::UnlockWeapon(const std::string_view InWeaponId)
{
	const auto Found = std::find_if(LockedWeapons.begin(), LockedWeapons.end(), [InWeaponId](const WeaponDef& InWeapon) { return InWeapon.Id == InWeaponId; });
	if ( Found == LockedWeapons.end() )
	{
		return false;
	}
	Weapons.push_back(*Found);
	ReloadTimers.push_back(0.0f);
	LockedWeapons.erase(Found);
	return true;
}

void Tank::AddAmmo(const int InAmount)
{
	if ( !bUnlimitedAmmo )
	{
		Ammo = std::min(GetMaxAmmo(), Ammo + InAmount);
	}
}

void Tank::SetAmmo(const int InAmount)
{
	if ( !bUnlimitedAmmo )
	{
		Ammo = std::max(0, std::min(GetMaxAmmo(), InAmount));
	}
}

int Tank::GetAmmo() const
{
	return Ammo;
}

int Tank::GetMaxAmmo() const
{
	return bUnlimitedAmmo ? -1 : Stats.GetInt(StatType::MaxAmmo);
}

bool Tank::HasUnlimitedAmmo() const
{
	return bUnlimitedAmmo;
}

int Tank::GetArmor() const
{
	return Stats.GetInt(StatType::Armor);
}

size_t Tank::GetWeaponCount() const
{
	return Weapons.size();
}

const WeaponDef& Tank::GetActiveWeapon() const
{
	static const WeaponDef None;
	return Weapons.empty() ? None : Weapons[ActiveWeapon];
}

size_t Tank::GetActiveWeaponIndex() const
{
	return ActiveWeapon;
}

bool Tank::SwitchWeapon()
{
	return Weapons.size() > 1 && SelectWeapon((ActiveWeapon + 1) % Weapons.size());
}

bool Tank::SelectWeapon(const size_t InIndex)
{
	if ( InIndex >= Weapons.size() )
	{
		return false;
	}
	ActiveWeapon = InIndex;
	return true;
}

bool Tank::CanFireActiveWeapon() const
{
	return !GetActiveWeapon().bUsesAmmo || bUnlimitedAmmo || Ammo > 0;
}

float Tank::GetActiveReloadTime() const
{
	const float Own = GetActiveWeapon().ReloadTime;
	return (Own > 0.0f ? Own : Stats.Get(StatType::ReloadTime)) * GetActiveWeapon().ReloadScale * ReloadDifficulty;
}

const std::string& Tank::GetDropTable() const
{
	return DropTable;
}

const std::string& Tank::GetSplitInto() const
{
	return SplitInto;
}

int Tank::GetSplitCount() const
{
	return SplitCount;
}

float Tank::GetSplitSpacingTiles() const
{
	return SplitSpacingTiles;
}

int Tank::GetShotDamage(const WeaponDef& InWeapon) const
{
	const int BaseDamage = (InWeapon.Damage > 0 ? InWeapon.Damage : GetDamage()) + InWeapon.DamageBonus;
	return std::max(1, static_cast<int>(std::lround(BaseDamage * DamageScale)));
}

TankAi* Tank::GetAi() const
{
	return Ai.get();
}

void Tank::ApplyDifficulty(const model::DifficultyDef& InMode)
{
	SetMaxHealth(static_cast<int>(std::lround(GetMaxHealth() * InMode.EnemyHealth)));
	DamageScale = std::max(0.0f, InMode.EnemyDamage);
	ReloadDifficulty = std::max(0.05f, InMode.ReloadTime);
	BulletSpeedScale = std::max(0.05f, InMode.BulletSpeed);
	bFriendlyFire = InMode.bFriendlyFire;
	CooldownScale = std::max(0.05f, InMode.AbilityCooldown);
	for ( size_t Index = 0; Index < Abilities.size(); ++Index )
	{
		AbilityDef Scaled = AbilityDefs[Index];
		Scaled.Cooldown *= CooldownScale;
		Abilities[Index]->Configure(Scaled);
	}
	if ( Ai )
	{
		Ai->ScaleBehaviour(InMode.AiThinkTime, InMode.AiFireRate, InMode.AiAimTolerance);
	}
}

bool Tank::ImproveWeapon(const std::string_view InWeaponId, const int InDamageBonus, const float InReloadScale)
{
	bool bFound = false;
	for ( std::vector<WeaponDef>* const List : { &Weapons, &LockedWeapons } )
	{
		for ( WeaponDef& Weapon : *List )
		{
			if ( Weapon.Id == InWeaponId )
			{
				Weapon.DamageBonus += InDamageBonus;
				Weapon.ReloadScale *= InReloadScale;
				bFound = true;
			}
		}
	}
	return bFound;
}

float Tank::GetReloadRemaining() const
{
	return ReloadTimers.empty() ? 0.0f : ReloadTimers[ActiveWeapon];
}

float Tank::GetReloadProgress() const
{
	const float CurrentReloadTime = GetActiveReloadTime();
	return CurrentReloadTime > 0.0f ? 1.0f - std::min(1.0f, GetReloadRemaining() / CurrentReloadTime) : 1.0f;
}

int Tank::GetDamage() const
{
	return Stats.GetInt(StatType::Damage);
}

EnemyType Tank::GetEnemyType() const
{
	return EnemyKind;
}

int Tank::GetScoreReward() const
{
	return ScoreReward;
}

int Tank::GetCoinReward() const
{
	return CoinReward;
}

void Tank::ApplyDamage(const int InDamage, const Team InSource)
{
	if ( InDamage <= 0 )
	{
		return;
	}

	const bool bAbsorbed = Buffs.IsInvulnerable() || (GetArmor() > 0 && InDamage <= GetArmor());
	GameContext::Get().GetManager<AudioManager>().Play(bAbsorbed ? ArmorSound : HitSound);
	if ( Buffs.IsInvulnerable() )
	{
		PlayShieldHit();
		return;
	}
	MovableObject::ApplyDamage(std::max(1, InDamage - GetArmor()), InSource);
}

void Tank::ApplyDamageFrom(const int InDamage, const Team InSource, const cocos2d::Vec2& InShotDirection)
{
	const model::ShotSide Side = model::ShotSideClassifier::Classify({ GetDirection().x, GetDirection().y }, { InShotDirection.x, InShotDirection.y });
	if ( Side == model::ShotSide::Front && bFrontalShield && InDamage > 0 )
	{
		GameContext::Get().GetManager<AudioManager>().Play(ArmorSound);
		PlayShieldHit();
		return;
	}
	const float Scale = Side == model::ShotSide::Back ? RearDamageScale : 1.0f;
	ApplyDamage(static_cast<int>(std::round(InDamage * Scale)), InSource);
}

bool Tank::HasFrontalShield() const
{
	return bFrontalShield;
}

void Tank::OnCommandReceived(const GameCommand& InCommand)
{
	switch ( InCommand.GetType() )
	{
		case CommandType::Shoot:
			Shoot();
			break;
		case CommandType::StartFiring:
			SetTriggerHeld(true);
			Shoot();
			break;
		case CommandType::StopFiring:
			SetTriggerHeld(false);
			break;
		case CommandType::UseAbility:
			UseSecondaryAbility();
			break;
		case CommandType::SwitchWeapon:
			SwitchWeapon();
			break;
		default:
			MovableObject::OnCommandReceived(InCommand);
			break;
	}
}

void Tank::Update(const float InDelta)
{
	MovableObject::Update(InDelta);

	for ( float& CurrentTimer : ReloadTimers )
	{
		CurrentTimer = std::max(0.0f, CurrentTimer - InDelta);
	}
	UpdateBurst(InDelta);
	UpdatePhases();
	for ( auto& CurrentAbility : Abilities )
	{
		CurrentAbility->Update(*this, InDelta);
	}
	if ( bTriggerHeld && CanAutoFire() )
	{
		Shoot();
	}
	if ( Buffs.Update(InDelta) )
	{
		RefreshStats();
	}
	if ( Ai && !bAiSuspended )
	{
		Ai->Update(InDelta);
	}
}

void Tank::SetAiSuspended(const bool bInSuspended)
{
	bAiSuspended = bInSuspended;
}

bool Tank::IsAiSuspended() const
{
	return bAiSuspended;
}

void Tank::OnTurnFinished()
{
	MovableObject::OnTurnFinished();
	if ( bShotQueued )
	{
		bShotQueued = false;
		Shoot();
	}
}

bool Tank::IsShotQueued() const
{
	return bShotQueued;
}

void Tank::OnBlocked()
{
	MovableObject::OnBlocked();
	if ( Ai && !bAiSuspended )
	{
		Ai->OnBlocked();
	}
}

void Tank::OnKilled()
{
	if ( auto CurrentVisualNode = GetVisualNode() )
	{
		effects::Explosion(CurrentVisualNode->getParent(), CurrentVisualNode->getPosition(), true);
	}
	GameContext::Get().GetManager<AudioManager>().Play(ExplodeSound);
}

} // namespace tanks

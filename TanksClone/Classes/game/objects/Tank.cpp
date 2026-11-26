#include "game/objects/Tank.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/GameConstants.h"
#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/EventReport.h"
#include "game/ObjectsFactory.h"
#include "model/LobFlight.h"
#include "model/ShotSide.h"
#include "model/SimulationClock.h"
#include "services/AudioManager.h"
#include "services/ConfigRefs.h"
#include "services/ResourceManager.h"
#include "ui/CocosConvert.h"
#include "templates/TContainers.h"

namespace game
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

FTankHandle Tank::Create(cocos2d::Node* const InNode)
{
	FTankHandle CurrentTank(new Tank());
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
	SetType(EObjectType::Vehicle);
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

	Weapons.Clear();
	LockedWeapons.Clear();
	for ( const WeaponDef& Weapon : InDef.Weapons )
	{
		(Weapon.bLocked ? LockedWeapons : Weapons).PushBack(Weapon);
	}
	BurstLeft = 0;
	HitSound = InDef.HitSound;
	ArmorSound = InDef.ArmorSound;
	ExplodeSound = InDef.ExplodeSound;
	ReloadTimers.Assign(Weapons.Size(), 0.0f);
	Heat.Assign(Weapons.Size(), 0.0f);
	SinceShot.Assign(Weapons.Size(), 0.0f);
	ActiveWeapon = 0;

	Abilities.Clear();
	AbilityDefs.Clear();
	for ( const AbilityDef& CurrentDef : InDef.Abilities )
	{
		AddAbility(CurrentDef);
	}
	for ( const std::string& CurrentId : InDef.PermanentBuffs )
	{
		AddPermanentBuff(CurrentId);
	}
	Phases = InDef.Phases;
	TArray<float> Thresholds;
	for ( const TankPhaseDef& Phase : Phases )
	{
		Thresholds.PushBack(Phase.HealthBelow);
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
	SideHits = model::DamageState(model::SideHitSettings::FromConfig(config_refs::GetConfig(ids::configs::SideHits)), GameContext::Get<model::SimulationClock>());
	Ai.Reset(InDef.Ai != EAiType::None ? new TankAi(*this, InDef.Ai) : nullptr);
	if ( Ai )
	{
		Ai->ApplyFeatures(InDef.AiFeatures);
	}

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
		Abilities.PushBack(std::move(CurrentAbility));
		AbilityDefs.PushBack(InDef);
	}
}

void Tank::AddPermanentBuff(const std::string& InId)
{
	if ( const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(InId) )
	{
		BuffDef Permanent = *Buff;
		Permanent.Duration = -1.0f;
		Buffs.Add(Permanent);
	}
}

bool Tank::HasPhases() const
{
	return !Phases.IsEmpty();
}

int Tank::GetPhase() const
{
	return PhaseState.GetCurrentPhase();
}

void Tank::SetOnPhaseCallback(const TFunction<void(Tank&, const TankPhaseDef&)>& InCallback)
{
	OnPhaseCallback = InCallback;
}

void Tank::UpdatePhases()
{
	if ( Phases.IsEmpty() || IsDestroying() || GetMaxHealth() <= 0 )
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
	GameContext::Get<AudioManager>().Play(InPhase.Sound);
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

	SetSpeed(Stats.Get(StatType::Speed) * SideHits.GetSpeedScale());
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
	return Abilities.Size();
}

const Ability* Tank::GetAbility(const size_t InIndex) const
{
	return InIndex < Abilities.Size() ? Abilities[InIndex].Get() : nullptr;
}

bool Tank::UseAbility(const size_t InIndex)
{
	return InIndex < Abilities.Size() && !IsDestroying() && Abilities[InIndex]->Activate(*this);
}

bool Tank::UseSecondaryAbility()
{
	for ( size_t Index = 1; Index < Abilities.Size(); ++Index )
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
	return !Weapons.IsEmpty() && GetActiveWeapon().bAutoFire;
}

bool Tank::Fire()
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode || !CurrentVisualNode->getParent() || IsDestroying() || Weapons.IsEmpty() || ReloadTimers[ActiveWeapon] > 0.0f
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
	SinceShot[ActiveWeapon] = 0.0f;
	if ( Weapon.HeatShots > 0 && ++Heat[ActiveWeapon] >= Weapon.HeatShots )
	{
		Heat[ActiveWeapon] = 0.0f;
		ReloadTimers[ActiveWeapon] = std::max(ReloadTimers[ActiveWeapon], Weapon.CoolTime);
	}
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
	Overrides["friendly_fire_share"] = FriendlyFireShare;
	Overrides["bullet_clash"] = bBulletClash;
	Overrides["team"] = GetTeam() == ETeam::Player ? "Player" : (GetTeam() == ETeam::Enemy ? "Enemy" : "Neutral");
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
		auto CurrentBullet = GameContext::Get<ObjectsFactory>().CreateFromConfig<PhysicsObject>(InWeapon.BulletConfig, CurrentVisualNode->getParent(), Overrides);
		if ( CurrentBullet && HasBounds() )
		{
			CurrentBullet->SetBounds(GetBounds());
		}
		bSpawned = bSpawned || CurrentBullet != nullptr;
	}
	if ( bSpawned )
	{
		effects::EffectStyle Flash;
		Flash.Width = InWeapon.FlashWidth;
		Flash.Length = InWeapon.FlashLength;
		Flash.Speed = InWeapon.FlashSpeed;
		Flash.Tint = view::cocos::ToCocos3B(InWeapon.FlashTint);
		GetLook().OnShot(*this, Muzzle, CurrentDirection, Flash);
		GameContext::Get<AudioManager>().Play(InWeapon.FireSound);
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
	GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
	{
		if ( &InOther == this || InOther.IsDestroying() || !InOther.GetVisualNode()
			|| InOther.GetTeam() == GetTeam() || InOther.GetTeam() == ETeam::Neutral )
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
	Weapons.PushBack(*Found);
	ReloadTimers.PushBack(0.0f);
	Heat.PushBack(0.0f);
	SinceShot.PushBack(0.0f);
	LockedWeapons.Erase(Found);
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
	return Weapons.Size();
}

const WeaponDef& Tank::GetActiveWeapon() const
{
	static const WeaponDef None;
	return Weapons.IsEmpty() ? None : Weapons[ActiveWeapon];
}

size_t Tank::GetActiveWeaponIndex() const
{
	return ActiveWeapon;
}

bool Tank::SwitchWeapon()
{
	return Weapons.Size() > 1 && SelectWeapon((ActiveWeapon + 1) % Weapons.Size());
}

bool Tank::SelectWeapon(const size_t InIndex)
{
	if ( InIndex >= Weapons.Size() )
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

float Tank::GetBulletSpeedScale() const
{
	return BulletSpeedScale;
}

TankAi* Tank::GetAi() const
{
	return Ai.Get();
}

void Tank::ApplyDifficulty(const model::DifficultyDef& InMode)
{
	SetMaxHealth(static_cast<int>(std::lround(GetMaxHealth() * InMode.EnemyHealth)));
	DamageScale = std::max(0.0f, InMode.EnemyDamage);
	ReloadDifficulty = std::max(0.05f, InMode.ReloadTime);
	BulletSpeedScale = std::max(0.05f, InMode.BulletSpeed);
	bFriendlyFire = InMode.bFriendlyFire;
	FriendlyFireShare = InMode.FriendlyFireShare;
	bBulletClash = InMode.bBulletClash;
	CooldownScale = std::max(0.05f, InMode.AbilityCooldown);
	for ( size_t Index = 0; Index < Abilities.Size(); ++Index )
	{
		AbilityDef Scaled = AbilityDefs[Index];
		Scaled.Cooldown *= CooldownScale;
		Abilities[Index]->Configure(Scaled);
	}
	if ( Ai )
	{
		Ai->ScaleBehaviour(InMode.AiThinkTime, InMode.AiFireRate, InMode.AiAimTolerance);
		Ai->ApplyFeatures(InMode.AiFeatures);
	}
}

bool Tank::ImproveWeapon(const std::string_view InWeaponId, const int InDamageBonus, const float InReloadScale)
{
	bool bFound = false;
	for ( TArray<WeaponDef>* const List : { &Weapons, &LockedWeapons } )
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
	return ReloadTimers.IsEmpty() ? 0.0f : ReloadTimers[ActiveWeapon];
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

EEnemyType Tank::GetEnemyType() const
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

void Tank::ApplyDamage(const int InDamage, const ETeam InSource)
{
	if ( InDamage <= 0 )
	{
		return;
	}

	const bool bAbsorbed = Buffs.IsInvulnerable() || (GetArmor() > 0 && InDamage <= GetArmor());
	GameContext::Get<AudioManager>().Play(bAbsorbed ? ArmorSound : HitSound);
	if ( Buffs.IsInvulnerable() )
	{
		PlayShieldHit();
		return;
	}
	MovableObject::ApplyDamage(std::max(1, InDamage - GetArmor()), InSource);
}

void Tank::ApplyDamageFrom(const int InDamage, const ETeam InSource, const cocos2d::Vec2& InShotDirection)
{
	const model::ShotSide Side = model::ShotSideClassifier::Classify({ GetDirection().x, GetDirection().y }, { InShotDirection.x, InShotDirection.y });
	if ( Side == model::ShotSide::Front && bFrontalShield && InDamage > 0 )
	{
		GameContext::Get<AudioManager>().Play(ArmorSound);
		PlayShieldHit();
		return;
	}
	events::Report("tank.hit");
	events::Report(Side == model::ShotSide::Front ? "tank.hit.front" : Side == model::ShotSide::Back ? "tank.hit.back" : "tank.hit.side");
	if ( GetTeam() == ETeam::Player )
	{
		events::Report("player.hit");
	}
	const float Scale = Side == model::ShotSide::Back ? RearDamageScale : 1.0f;
	const int HealthBefore = GetHealth();
	ApplyDamage(static_cast<int>(std::round(InDamage * Scale)), InSource);
	if ( GetHealth() < HealthBefore )
	{
		GetLook().AddHit(*this, InShotDirection);
		if ( Side == model::ShotSide::Side )
		{
			SideHits.OnSideHit(HealthBefore - GetHealth(), GetMaxHealth());
			RefreshStats();
		}
	}
}

bool Tank::HasFrontalShield() const
{
	return bFrontalShield;
}

void Tank::OnCommandReceived(const GameCommand& InCommand)
{
	switch ( InCommand.GetType() )
	{
		case ECommandType::Shoot:
			Shoot();
			break;
		case ECommandType::StartFiring:
			SetTriggerHeld(true);
			Shoot();
			break;
		case ECommandType::StopFiring:
			SetTriggerHeld(false);
			break;
		case ECommandType::UseAbility:
			UseSecondaryAbility();
			break;
		case ECommandType::SwitchWeapon:
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

	for ( size_t Index = 0; Index < Heat.Size(); ++Index )
	{
		const WeaponDef& Weapon = Weapons[Index];
		SinceShot[Index] += InDelta;
		if ( Weapon.HeatShots > 0 && SinceShot[Index] > Weapon.CoolDelay )
		{
			Heat[Index] = std::max(0.0f, Heat[Index] - InDelta * Weapon.HeatShots / std::max(Weapon.CoolTime, 0.01f));
		}
	}
	for ( float& CurrentTimer : ReloadTimers )
	{
		CurrentTimer = std::max(0.0f, CurrentTimer - InDelta);
	}
	UpdateBurst(InDelta);
	UpdatePhases();
	if ( bFrozen )
	{
		GetLook().Update(*this);
		return;
	}
	for ( auto& CurrentAbility : Abilities )
	{
		CurrentAbility->Update(*this, InDelta);
	}
	if ( bTriggerHeld && CanAutoFire() )
	{
		Shoot();
	}
	const bool bSideHitsChanged = SideHits.Update();
	if ( Buffs.Update(InDelta) || bSideHitsChanged )
	{
		RefreshStats();
	}
	if ( Ai && !bAiSuspended )
	{
		Ai->Update(InDelta);
	}
	GetLook().Update(*this);
}

TankLook& Tank::GetLook()
{
	if ( !Look )
	{
		Look = MakeUnique<TankLook>(TankLookSettings::Load());
	}
	return *Look;
}

void Tank::SetFriendlyFire(const bool bInFriendlyFire)
{
	bFriendlyFire = bInFriendlyFire;
}

void Tank::SetBulletClash(const bool bInClash)
{
	bBulletClash = bInClash;
}

bool Tank::IsImmobilized() const
{
	return SideHits.IsImmobilized();
}

void Tank::SetFrozen(const bool bInFrozen)
{
	bFrozen = bInFrozen;
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
		GetLook().PlayDestroyed(*this);
	}
	GameContext::Get<AudioManager>().Play(ExplodeSound);
}

} // namespace game

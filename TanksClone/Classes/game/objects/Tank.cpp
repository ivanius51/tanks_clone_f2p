#include "game/objects/Tank.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/GameConstants.h"
#include "game/Effects.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"

namespace tanks
{

namespace
{
	constexpr float MuzzleDistance = TileSize * 0.6f;
	constexpr float RadiansToDegrees = 180.0f / 3.14159265358979323846f;
	constexpr int ShieldTag = 9001;
	constexpr float ShieldRadius = 25.0f; // in sprite pixels (the sprite is 32x32)
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

	Weapons = InDef.Weapons;
	ReloadTimers.assign(Weapons.size(), 0.0f);
	ActiveWeapon = 0;

	Abilities.clear();
	for ( const AbilityDef& CurrentDef : InDef.Abilities )
	{
		if ( auto CurrentAbility = CreateAbility(CurrentDef.Type) )
		{
			CurrentAbility->Configure(CurrentDef);
			Abilities.push_back(std::move(CurrentAbility));
		}
	}

	for ( const std::string& CurrentId : InDef.PermanentBuffs )
	{
		if ( const BuffDef* Buff = BuffCatalog::GetInstance().Find(CurrentId) )
		{
			BuffDef Permanent = *Buff;
			Permanent.Duration = -1.0f;
			Buffs.Add(Permanent);
		}
	}

	EnemyKind = InDef.EnemyKind;
	ScoreReward = InDef.ScoreReward;
	CoinReward = InDef.CoinReward;
	Ai.reset(InDef.Ai != AiType::None ? new TankAi(*this, InDef.Ai) : nullptr);

	RefreshStats();
	RestoreFull();
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
	RescaleMaxHealth(Stats.GetInt(StatType::MaxHealth));
	if ( !bUnlimitedAmmo )
	{
		Ammo = std::min(Ammo, GetMaxAmmo());
	}
	UpdateShieldVisual();
}

// A translucent bubble around the tank while it is invulnerable.
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

bool Tank::Fire()
{
	auto CurrentVisualNode = GetVisualNode();
	if ( !CurrentVisualNode || !CurrentVisualNode->getParent() || IsDestroying() || Weapons.empty() || ReloadTimers[ActiveWeapon] > 0.0f
		|| !CanFireActiveWeapon() )
	{
		return false;
	}

	const WeaponDef& Weapon = GetActiveWeapon();
	const cocos2d::Vec2& CurrentDirection = GetDirection();
	const cocos2d::Vec2 Muzzle = CurrentVisualNode->getPosition() + CurrentDirection * MuzzleDistance;

	cocos2d::ValueVector Position;
	Position.push_back(cocos2d::Value(Muzzle.x));
	Position.push_back(cocos2d::Value(Muzzle.y));

	cocos2d::ValueMap Overrides;
	Overrides["position"] = Position;
	Overrides["rotation"] = std::atan2(CurrentDirection.x, CurrentDirection.y) * RadiansToDegrees;
	Overrides["damage"] = Weapon.Damage > 0 ? Weapon.Damage : GetDamage();
	Overrides["team"] = GetTeam() == Team::Player ? "Player" : (GetTeam() == Team::Enemy ? "Enemy" : "Neutral");

	auto CurrentBullet = ObjectsFactory::GetInstance().CreateFromConfig<PhysicsObject>(Weapon.BulletConfig, CurrentVisualNode->getParent(), Overrides);
	if ( !CurrentBullet )
	{
		return false;
	}
	if ( HasBounds() )
	{
		CurrentBullet->SetBounds(GetBounds());
	}

	if ( Weapon.bUsesAmmo && !bUnlimitedAmmo )
	{
		--Ammo;
	}
	ReloadTimers[ActiveWeapon] = GetActiveReloadTime();

	if ( GetTeam() == Team::Player )
	{
		AudioManager::GetInstance().Play(Sound::PlayerShot);
	}
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
	return Own > 0.0f ? Own : Stats.Get(StatType::ReloadTime);
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
	AudioManager::GetInstance().Play(bAbsorbed ? Sound::HitArmor : Sound::HitWall);
	if ( Buffs.IsInvulnerable() )
	{
		return;
	}
	MovableObject::ApplyDamage(std::max(1, InDamage - GetArmor()), InSource);
}

void Tank::OnCommandReceived(const GameCommand& InCommand)
{
	switch ( InCommand.GetType() )
	{
		case CommandType::Shoot:
			Shoot();
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
	for ( auto& CurrentAbility : Abilities )
	{
		CurrentAbility->Update(*this, InDelta);
	}
	if ( Buffs.Update(InDelta) )
	{
		RefreshStats();
	}
	if ( Ai )
	{
		Ai->Update(InDelta);
	}
}

void Tank::OnBlocked()
{
	MovableObject::OnBlocked();
	if ( Ai )
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
	if ( GetTeam() == Team::Player )
	{
		AudioManager::GetInstance().Play(Sound::PlayerExplode);
	}
	else
	{
		AudioManager::GetInstance().Play(EnemyKind == EnemyType::Boss ? Sound::BossExplode : Sound::EnemyExplode);
	}
}

} // namespace tanks

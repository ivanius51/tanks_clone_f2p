#include "game/components/Ability.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/Effects.h"
#include "game/ObjectsFactory.h"
#include "game/components/Buff.h"
#include "game/components/Weapon.h"
#include "game/objects/Tank.h"
#include "model/ChargeState.h"
#include "model/DropTimer.h"
#include "model/RepairPicker.h"
#include "services/AudioManager.h"

namespace tanks
{

void Ability::Configure(const AbilityDef& /*InDef*/)
{
}

void Ability::Update(Tank& /*InOwner*/, float /*InDelta*/)
{
}

float Ability::GetReadiness() const
{
	return 1.0f;
}

float Ability::GetCooldownRemaining() const
{
	return 0.0f;
}

std::string Ability::GetIconFrame() const
{
	return "icon_shoot";
}

cocos2d::Color3B Ability::GetIconColor() const
{
	return cocos2d::Color3B::WHITE;
}

namespace
{

// Fires a bullet. Ammo and reloading belong to the tank itself.
class ShootAbility : public Ability
{
public:
	bool Activate(Tank& InOwner) override
	{
		Owner = &InOwner;
		return InOwner.Fire();
	}

	float GetReadiness() const override
	{
		return Owner ? Owner->GetReloadProgress() : 1.0f;
	}

	float GetCooldownRemaining() const override
	{
		return Owner ? Owner->GetReloadRemaining() : 0.0f;
	}

	std::string GetTitleKey() const override
	{
		return Owner ? Owner->GetActiveWeapon().NameKey : std::string("ABILITY_SHOOT");
	}

	std::string GetIconFrame() const override
	{
		return Owner ? Owner->GetActiveWeapon().Icon : Ability::GetIconFrame();
	}

	void Update(Tank& InOwner, float) override
	{
		Owner = &InOwner;
	}

private:
	const Tank* Owner = nullptr;
};

// Gives the tank a buff from the catalog, then needs time to recharge.
class BuffAbility : public Ability
{
public:
	void Configure(const AbilityDef& InDef) override
	{
		BuffId = InDef.BuffId;
		Cooldown = InDef.Cooldown;
		ActivateSound = InDef.ActivateSound;
	}

	void Update(Tank&, const float InDelta) override
	{
		Timer = std::max(0.0f, Timer - InDelta);
	}

	bool Activate(Tank& InOwner) override
	{
		const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(BuffId);
		if ( Timer > 0.0f || !Buff )
		{
			return false;
		}

		InOwner.GetBuffs().Add(*Buff);
		InOwner.RefreshStats();
		Timer = Cooldown;
		GameContext::Get().GetManager<AudioManager>().Play(ActivateSound);
		return true;
	}

	float GetReadiness() const override
	{
		return Cooldown > 0.0f ? 1.0f - std::min(1.0f, Timer / Cooldown) : 1.0f;
	}

	float GetCooldownRemaining() const override
	{
		return Timer;
	}

	std::string GetIconFrame() const override
	{
		const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(BuffId);
		return Buff ? Buff->Icon : Ability::GetIconFrame();
	}

	cocos2d::Color3B GetIconColor() const override
	{
		const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(BuffId);
		return Buff ? Buff->IconColor : cocos2d::Color3B::WHITE;
	}

	std::string GetTitleKey() const override
	{
		const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(BuffId);
		return Buff ? Buff->NameKey : BuffId;
	}

private:
	std::string BuffId;
	float Cooldown = 10.0f;
	float Timer = 0.0f;
	model::ResourceId ActivateSound;
};

// Rams straight ahead at an enemy in line, hurts the tank it hits, then stands stunned. The AI is paused meanwhile.
class ChargeAbility : public Ability
{
public:
	void Configure(const AbilityDef& InDef) override
	{
		Def = InDef;
		State.Configure(InDef.Duration, InDef.StunTime, InDef.Cooldown);
	}

	void Update(Tank& InOwner, const float InDelta) override
	{
		const bool bWasCharging = State.IsCharging();
		State.Update(InDelta);
		if ( State.GetPhase() == model::ChargeState::Phase::Ready && HasTargetAhead(InOwner) )
		{
			Activate(InOwner);
		}
		if ( State.IsCharging() )
		{
			UpdateCharge(InOwner);
		}
		if ( bWasCharging && !State.IsCharging() )
		{
			BeginStun(InOwner);
		}
		if ( !State.IsCharging() && !State.IsStunned() && InOwner.IsAiSuspended() )
		{
			InOwner.SetAiSuspended(false);
		}
	}

	bool Activate(Tank& InOwner) override
	{
		if ( InOwner.IsTurning() || !State.Start() )
		{
			return false;
		}
		AddBuff(InOwner, Def.BuffId);
		InOwner.SetAiSuspended(true);
		InOwner.Move();
		GameContext::Get().GetManager<AudioManager>().Play(Def.ActivateSound);
		return true;
	}

	float GetReadiness() const override
	{
		return State.GetReadiness();
	}

	float GetCooldownRemaining() const override
	{
		return State.GetRemaining();
	}

	std::string GetTitleKey() const override
	{
		return "ABILITY_CHARGE";
	}

	std::string GetIconFrame() const override
	{
		return "icon_speed";
	}

private:
	static constexpr float ContactProbe = 6.0f;
	static constexpr float LineHalfWidthTiles = 0.5f;

	static void AddBuff(Tank& InOwner, const std::string& InId)
	{
		if ( const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(InId) )
		{
			InOwner.GetBuffs().Add(*Buff);
			InOwner.RefreshStats();
		}
	}

	static bool IsFoe(const Tank& InOwner, const Tank& InOther)
	{
		return &InOther != &InOwner && !InOther.IsDestroying() && InOther.GetVisualNode()
			&& InOther.GetTeam() != InOwner.GetTeam() && InOther.GetTeam() != Team::Neutral;
	}

	bool HasTargetAhead(const Tank& InOwner) const
	{
		if ( !InOwner.GetVisualNode() || InOwner.IsTurning() )
		{
			return false;
		}
		const cocos2d::Vec2 Origin = InOwner.GetVisualNode()->getPosition();
		const cocos2d::Vec2 Ahead = InOwner.GetDirection();
		bool bFound = false;
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
		{
			if ( bFound || !IsFoe(InOwner, InOther) )
			{
				return;
			}
			const cocos2d::Vec2 Offset = InOther.GetVisualNode()->getPosition() - Origin;
			const float Along = Offset.dot(Ahead);
			const float Across = std::fabs(Offset.cross(Ahead));
			bFound = Along > 0.0f && Along <= Def.RangeTiles * TileSize && Across <= LineHalfWidthTiles * TileSize;
		});
		return bFound;
	}

	void UpdateCharge(Tank& InOwner)
	{
		if ( !InOwner.IsRunning() )
		{
			InOwner.Move();
		}
		const cocos2d::Rect Probe = InOwner.GetCollisionRectAt(InOwner.GetVisualNode()->getPosition() + InOwner.GetDirection() * ContactProbe);
		Tank* Victim = nullptr;
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
		{
			if ( !Victim && IsFoe(InOwner, InOther) && Probe.intersectsRect(InOther.GetCollisionRect()) )
			{
				Victim = &InOther;
			}
		});
		if ( Victim )
		{
			Victim->ApplyDamage(Def.ContactDamage, InOwner.GetTeam());
			State.Hit();
		}
		else if ( InOwner.IsBlocked() )
		{
			State.Hit();
		}
	}

	void BeginStun(Tank& InOwner)
	{
		InOwner.GetBuffs().Remove(Def.BuffId);
		AddBuff(InOwner, Def.StunBuffId);
		InOwner.RefreshStats();
		InOwner.Stop();
	}

	AbilityDef Def;
	model::ChargeState State;
};

// Every cooldown fixes the most hurt allies near the owner; it works by itself, without a command.
class RepairAbility : public Ability
{
public:
	void Configure(const AbilityDef& InDef) override
	{
		Def = InDef;
		Timer = Def.Cooldown;
	}

	void Update(Tank& InOwner, const float InDelta) override
	{
		Timer = std::max(0.0f, Timer - InDelta);
		if ( Timer <= 0.0f )
		{
			Activate(InOwner);
		}
	}

	bool Activate(Tank& InOwner) override
	{
		if ( Timer > 0.0f || !InOwner.GetVisualNode() || InOwner.IsDestroying() )
		{
			return false;
		}
		const cocos2d::Vec2 Origin = InOwner.GetVisualNode()->getPosition();
		std::vector<Tank*> Allies;
		std::vector<model::RepairCandidate> Candidates;
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
		{
			if ( &InOther != &InOwner && !InOther.IsDestroying() && InOther.GetVisualNode() && InOther.GetTeam() == InOwner.GetTeam() )
			{
				Allies.push_back(&InOther);
				Candidates.push_back({ InOther.GetHealth(), InOther.GetMaxHealth(), InOther.GetVisualNode()->getPosition().distance(Origin) });
			}
		});
		const std::vector<int> Picked = model::RepairPicker::Pick(Candidates, Def.RangeTiles * TileSize, Def.MaxTargets);
		if ( Picked.empty() )
		{
			return false;
		}
		for ( const int Index : Picked )
		{
			Allies[Index]->Heal(Def.HealAmount);
			effects::RepairPulse(Allies[Index]->GetVisualNode()->getParent(), Allies[Index]->GetVisualNode()->getPosition());
		}
		Timer = Def.Cooldown;
		GameContext::Get().GetManager<AudioManager>().Play(Def.ActivateSound);
		return true;
	}

	float GetReadiness() const override
	{
		return Def.Cooldown > 0.0f ? 1.0f - std::min(1.0f, Timer / Def.Cooldown) : 1.0f;
	}

	float GetCooldownRemaining() const override
	{
		return Timer;
	}

	std::string GetTitleKey() const override
	{
		return "ABILITY_REPAIR";
	}

private:
	AbilityDef Def;
	float Timer = 0.0f;
};

// Leaves a mine behind every few seconds of driving; it works by itself, without a command.
class LayMineAbility : public Ability
{
public:
	void Configure(const AbilityDef& InDef) override
	{
		Mine.Id = "laid_mine";
		Mine.BulletConfig = InDef.BulletConfig;
		Mine.Damage = InDef.ShotDamage;
		Mine.bDropsBehind = true;
		Mine.FireSound = InDef.ActivateSound;
		Timer.Configure(InDef.Cooldown);
	}

	void Update(Tank& InOwner, const float InDelta) override
	{
		if ( Timer.Update(InDelta, InOwner.IsRunning() && !InOwner.IsTurning()) )
		{
			Activate(InOwner);
		}
	}

	bool Activate(Tank& InOwner) override
	{
		return !Mine.BulletConfig.empty() && InOwner.FireExtra(Mine);
	}

	float GetReadiness() const override
	{
		return Timer.GetReadiness();
	}

	float GetCooldownRemaining() const override
	{
		return Timer.GetRemaining();
	}

	std::string GetTitleKey() const override
	{
		return "ABILITY_LAY_MINE";
	}

private:
	WeaponDef Mine;
	model::DropTimer Timer;
};

template <typename T>
std::unique_ptr<Ability> Make()
{
	return std::unique_ptr<Ability>(new T());
}

struct AbilityEntry
{
	const char* TypeName;
	std::unique_ptr<Ability> (*Create)();
};

// The one place that lists the ability types.
const std::array<AbilityEntry, 5> Abilities = { {
	{ "Shoot", &Make<ShootAbility> },
	{ "Buff", &Make<BuffAbility> },
	{ "Charge", &Make<ChargeAbility> },
	{ "Repair", &Make<RepairAbility> },
	{ "LayMine", &Make<LayMineAbility> }
} };

} // namespace

std::unique_ptr<Ability> CreateAbility(const std::string& InType)
{
	for ( const auto& CurrentEntry : Abilities )
	{
		if ( InType == CurrentEntry.TypeName )
		{
			return CurrentEntry.Create();
		}
	}
	CCLOGERROR("Ability: unknown type %s", InType.c_str());
	return nullptr;
}

} // namespace tanks

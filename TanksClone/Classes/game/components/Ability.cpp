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
#include "model/CloakCycle.h"
#include "model/Cooldown.h"
#include "model/DropTimer.h"
#include "model/SimulationClock.h"
#include "model/RepairPicker.h"
#include "services/AudioManager.h"
#include "templates/TContainers.h"

namespace game
{

double Ability::Now()
{
	return GameContext::Get<model::SimulationClock>().GetNow();
}

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

	bool Activate(Tank& InOwner) override
	{
		const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(BuffId);
		if ( !Timer.IsReady(Now()) || !Buff )
		{
			return false;
		}

		InOwner.GetBuffs().Add(*Buff);
		InOwner.RefreshStats();
		Timer.Start(Now(), Cooldown);
		GameContext::Get<AudioManager>().Play(ActivateSound);
		return true;
	}

	float GetReadiness() const override
	{
		return Timer.GetReadiness(Now());
	}

	float GetCooldownRemaining() const override
	{
		return Timer.GetRemaining(Now());
	}

	std::string GetIconFrame() const override
	{
		const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(BuffId);
		return Buff ? Buff->Icon : Ability::GetIconFrame();
	}

	cocos2d::Color3B GetIconColor() const override
	{
		const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(BuffId);
		return Buff ? Buff->IconColor : cocos2d::Color3B::WHITE;
	}

	std::string GetTitleKey() const override
	{
		const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(BuffId);
		return Buff ? Buff->NameKey : BuffId;
	}

private:
	std::string BuffId;
	float Cooldown = 10.0f;
	model::Cooldown Timer;
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
		GameContext::Get<AudioManager>().Play(Def.ActivateSound);
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
		if ( const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(InId) )
		{
			InOwner.GetBuffs().Add(*Buff);
			InOwner.RefreshStats();
		}
	}

	static bool IsFoe(const Tank& InOwner, const Tank& InOther)
	{
		return &InOther != &InOwner && !InOther.IsDestroying() && InOther.GetVisualNode()
			&& InOther.GetTeam() != InOwner.GetTeam() && InOther.GetTeam() != ETeam::Neutral;
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
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
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
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
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
		Timer.Start(Now(), Def.Cooldown);
	}

	void Update(Tank& InOwner, const float InDelta) override
	{
		if ( Timer.IsReady(Now()) )
		{
			Activate(InOwner);
		}
	}

	bool Activate(Tank& InOwner) override
	{
		if ( !Timer.IsReady(Now()) || !InOwner.GetVisualNode() || InOwner.IsDestroying() )
		{
			return false;
		}
		const cocos2d::Vec2 Origin = InOwner.GetVisualNode()->getPosition();
		TArray<Tank*> Allies;
		TArray<model::RepairCandidate> Candidates;
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&](Tank& InOther)
		{
			if ( &InOther != &InOwner && !InOther.IsDestroying() && InOther.GetVisualNode() && InOther.GetTeam() == InOwner.GetTeam() )
			{
				Allies.PushBack(&InOther);
				Candidates.PushBack({ InOther.GetHealth(), InOther.GetMaxHealth(), InOther.GetVisualNode()->getPosition().distance(Origin) });
			}
		});
		const TArray<int> Picked = model::RepairPicker::Pick(Candidates, Def.RangeTiles * TileSize, Def.MaxTargets);
		if ( Picked.IsEmpty() )
		{
			return false;
		}
		for ( const int Index : Picked )
		{
			Allies[Index]->Heal(Def.HealAmount);
			effects::RepairPulse(Allies[Index]->GetVisualNode()->getParent(), Allies[Index]->GetVisualNode()->getPosition());
		}
		Timer.Start(Now(), Def.Cooldown);
		GameContext::Get<AudioManager>().Play(Def.ActivateSound);
		return true;
	}

	float GetReadiness() const override
	{
		return Timer.GetReadiness(Now());
	}

	float GetCooldownRemaining() const override
	{
		return Timer.GetRemaining(Now());
	}

	std::string GetTitleKey() const override
	{
		return "ABILITY_REPAIR";
	}

private:
	AbilityDef Def;
	model::Cooldown Timer;
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

// Fades the tank out of sight and back, over and over; a hit shows it at once. Works by itself.
// Cracks, holes and lamps sit in nested layers that are added later; every level must pass the fade down.
void EnableCascadeOpacity(cocos2d::Node& InNode)
{
	InNode.setCascadeOpacityEnabled(true);
	for ( cocos2d::Node* const Child : InNode.getChildren() )
	{
		EnableCascadeOpacity(*Child);
	}
}

class CloakAbility : public Ability
{
public:
	void Configure(const AbilityDef& InDef) override
	{
		Cycle.Configure(InDef.Duration, InDef.Cooldown, InDef.FadeTime, InDef.HiddenOpacity);
		ActivateSound = InDef.ActivateSound;
	}

	void Update(Tank& InOwner, const float InDelta) override
	{
		auto CurrentVisualNode = InOwner.GetVisualNode();
		if ( !CurrentVisualNode )
		{
			return;
		}
		if ( LastHealth >= 0 && InOwner.GetHealth() < LastHealth )
		{
			Cycle.Reveal();
		}
		LastHealth = InOwner.GetHealth();
		const bool bWasHidden = Cycle.IsHidden();
		Cycle.Update(InDelta);
		if ( Cycle.IsHidden() && !bWasHidden )
		{
			GameContext::Get<AudioManager>().Play(ActivateSound);
		}
		EnableCascadeOpacity(*CurrentVisualNode);
		CurrentVisualNode->setOpacity(static_cast<GLubyte>(Cycle.GetOpacity() * 255.0f));
	}

	bool Activate(Tank&) override
	{
		return false;
	}

	float GetReadiness() const override
	{
		return Cycle.IsHidden() ? 1.0f : 0.0f;
	}

	std::string GetTitleKey() const override
	{
		return "ABILITY_CLOAK";
	}

private:
	model::CloakCycle Cycle;
	model::ResourceId ActivateSound;
	int LastHealth = -1;
};

template <typename T>
TUniquePtr<Ability> Make()
{
	return TUniquePtr<Ability>(new T());
}

struct AbilityEntry
{
	const char* TypeName;
	TUniquePtr<Ability> (*Create)();
};

// The one place that lists the ability types.
const std::array<AbilityEntry, 6> Abilities = { {
	{ "Shoot", &Make<ShootAbility> },
	{ "Buff", &Make<BuffAbility> },
	{ "Charge", &Make<ChargeAbility> },
	{ "Repair", &Make<RepairAbility> },
	{ "LayMine", &Make<LayMineAbility> },
	{ "Cloak", &Make<CloakAbility> }
} };

} // namespace

TUniquePtr<Ability> CreateAbility(const std::string& InType)
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

} // namespace game

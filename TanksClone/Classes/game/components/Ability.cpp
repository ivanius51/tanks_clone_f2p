#include "game/components/Ability.h"

#include <algorithm>
#include <array>
#include "game/components/Buff.h"
#include "core/DataConverter.h"
#include "game/objects/Tank.h"

namespace tanks
{

void Ability::Configure(const cocos2d::ValueMap& /*InData*/)
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
// Keys: "buff" (buff id), "cooldown" (seconds).
class BuffAbility : public Ability
{
public:
	void Configure(const cocos2d::ValueMap& InData) override
	{
		BuffId = DataConverter::GetString(InData, "buff", "");
		Cooldown = DataConverter::GetFloat(InData, "cooldown", Cooldown);
	}

	void Update(Tank&, const float InDelta) override
	{
		Timer = std::max(0.0f, Timer - InDelta);
	}

	bool Activate(Tank& InOwner) override
	{
		const BuffDef* Buff = BuffCatalog::GetInstance().Find(BuffId);
		if ( Timer > 0.0f || !Buff )
		{
			return false;
		}

		InOwner.GetBuffs().Add(*Buff);
		InOwner.RefreshStats();
		Timer = Cooldown;
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
		const BuffDef* Buff = BuffCatalog::GetInstance().Find(BuffId);
		return Buff ? Buff->Icon : Ability::GetIconFrame();
	}

	cocos2d::Color3B GetIconColor() const override
	{
		const BuffDef* Buff = BuffCatalog::GetInstance().Find(BuffId);
		return Buff ? Buff->IconColor : cocos2d::Color3B::WHITE;
	}

	std::string GetTitleKey() const override
	{
		const BuffDef* Buff = BuffCatalog::GetInstance().Find(BuffId);
		return Buff ? Buff->NameKey : BuffId;
	}

private:
	std::string BuffId;
	float Cooldown = 10.0f;
	float Timer = 0.0f;
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
const std::array<AbilityEntry, 2> Abilities = { {
	{ "Shoot", &Make<ShootAbility> },
	{ "Buff", &Make<BuffAbility> }
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

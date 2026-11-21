#pragma once

#include <memory>
#include <string>

#include "cocos2d.h"

#include "model/ResourceId.h"

namespace tanks
{

class Tank;

/** One entry of the "abilities" list of a tank config. */
struct AbilityDef
{
	std::string Type;       // "Shoot", "Buff", "Charge"
	std::string BuffId;     // for "Buff"; for "Charge" the buff held while charging
	float Cooldown = 10.0f; // seconds, for "Buff" and "Charge"
	model::ResourceId ActivateSound; // for "Buff" and "Charge"; invalid = silent
	// "Charge": rams straight ahead when an enemy is in line within RangeTiles, for at most Duration seconds;
	// hitting a tank deals ContactDamage, and any end of the charge stuns the rammer (StunBuffId) for StunTime.
	std::string StunBuffId;
	float Duration = 1.0f;
	float StunTime = 1.5f;
	int ContactDamage = 3;
	float RangeTiles = 6.0f;
};

// Something a tank can do on command: fire, raise a shield... Abilities are listed in
// the tank's config ("abilities": [{"type": "Shoot"}, {"type": "Buff", "buff": "shield"}]),
// so enemies and the player get their skills from data.
// The first ability of a tank is its primary one (fired with the fire button / by AI shooting).
//
// To add an ability: derive from Ability and add one line to the table in Ability.cpp.
class Ability
{
public:
	virtual ~Ability() = default;

	virtual void Configure(const AbilityDef& InDef);
	virtual void Update(Tank& InOwner, float InDelta);

	// Tries to use the ability. Returns true if it was used.
	virtual bool Activate(Tank& InOwner) = 0;
	// 0 = just used, 1 = ready to use again.
	virtual float GetReadiness() const;
	// Seconds until the ability can be used again (0 when ready).
	virtual float GetCooldownRemaining() const;
	// String id of the title shown in the HUD.
	virtual std::string GetTitleKey() const = 0;
	// Sprite frame and tint of the icon in the HUD.
	virtual std::string GetIconFrame() const;
	virtual cocos2d::Color3B GetIconColor() const;
};

// Creates an ability by its type name ("Shoot", "Buff"). Returns nullptr for unknown types.
std::unique_ptr<Ability> CreateAbility(const std::string& InType);

} // namespace tanks

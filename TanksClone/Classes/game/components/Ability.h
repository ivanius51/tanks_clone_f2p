#pragma once

#include <memory>
#include <string>
#include "cocos2d.h"

namespace tanks
{

class Tank;

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

	// Reads the parameters of this ability from its config entry.
	virtual void Configure(const cocos2d::ValueMap& InData);
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

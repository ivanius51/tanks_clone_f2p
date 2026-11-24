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
	std::string Type;
	std::string BuffId;
	float Cooldown = 10.0f;
	model::ResourceId ActivateSound;
	std::string StunBuffId;
	float Duration = 1.0f;
	float StunTime = 1.5f;
	int ContactDamage = 3;
	float RangeTiles = 6.0f;
	int HealAmount = 2;
	int MaxTargets = 2;
	std::string BulletConfig;
	int ShotDamage = 0;
	float FadeTime = 0.4f;
	float HiddenOpacity = 0.1f;
};

/**
 * Something a tank can do on command: fire, raise a shield... Abilities are listed in
 * the tank's config ("abilities": [{"type": "Shoot"}, {"type": "Buff", "buff": "shield"}]),
 * so enemies and the player get their skills from data.
 * The first ability of a tank is its primary one (fired with the fire button / by AI shooting).
 *
 * To add an ability: derive from Ability and add one line to the table in Ability.cpp.
 */
class Ability
{
public:
	virtual ~Ability() = default;

	virtual void Configure(const AbilityDef& InDef);
	virtual void Update(Tank& InOwner, float InDelta);

	virtual bool Activate(Tank& InOwner) = 0;
	virtual float GetReadiness() const;
	virtual float GetCooldownRemaining() const;
	virtual std::string GetTitleKey() const = 0;
	virtual std::string GetIconFrame() const;
	virtual cocos2d::Color3B GetIconColor() const;

protected:
	static double Now();
};

std::unique_ptr<Ability> CreateAbility(const std::string& InType);
} // namespace tanks

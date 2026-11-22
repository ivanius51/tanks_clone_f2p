#pragma once

#include <functional>
#include <memory>
#include <string>

#include "game/objects/PhysicsObject.h"

namespace tanks
{

// A pickup lying on the map. The player's tank collects it by driving over it.
class Bonus : public PhysicsObject
{
public:
	static std::unique_ptr<Bonus> Create(cocos2d::Node* InNode);

	// Keys: everything of GameObject plus "bonus_type" ("Ammo" / "Repair" / "Buff" / "Coins"), "amount", for Buff "buff",
	// and "lifetime" (seconds, 0 = stays) with "blink_time" / "blink_period" for the blinking before it vanishes.
	void Configure(const cocos2d::ValueMap& InData) override;
	// Called with the amount when a coin pickup is collected.
	void SetOnCoins(const std::function<void(int)>& InOnCoins);
	void SetOnPicked(const std::function<void()>& InOnPicked);

protected:
	void Update(float InDelta) override;

private:
	Bonus() = default;

	bool Build() override;

	BonusType BonusKind = BonusType::Undefined;
	int Amount = 1;
	std::string BuffId;
	std::string PickupEffect;
	float Lifetime = 0.0f;
	float BlinkTime = 0.0f;
	float BlinkPeriod = 0.15f;
	float Age = 0.0f;
	std::function<void(int)> OnCoins;
	std::function<void()> OnPicked;
};

} // namespace tanks

#pragma once

#include <memory>
#include <string>

#include "game/objects/PhysicsObject.h"
#include "templates/TFunction.h"

namespace tanks
{
/** A pickup lying on the map. The player's tank collects it by driving over it. */
class Bonus : public PhysicsObject
{
public:
	static std::unique_ptr<Bonus> Create(cocos2d::Node* InNode);

	void Configure(const cocos2d::ValueMap& InData) override;
	void SetOnCoins(const TFunction<void(int)>& InOnCoins);
	void SetOnPicked(const TFunction<void()>& InOnPicked);

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
	TFunction<void(int)> OnCoins;
	TFunction<void()> OnPicked;
};
} // namespace tanks

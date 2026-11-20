#pragma once

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

	// Keys: everything of GameObject plus "bonus_type" ("Ammo" / "Repair" / "Buff"), "amount" and, for Buff, "buff".
	void Configure(const cocos2d::ValueMap& InData) override;

protected:
	void Update(float InDelta) override;

private:
	Bonus() = default;

	bool Build() override;

	BonusType BonusKind = BonusType::Undefined;
	int Amount = 1;
	std::string BuffId;
};

} // namespace tanks

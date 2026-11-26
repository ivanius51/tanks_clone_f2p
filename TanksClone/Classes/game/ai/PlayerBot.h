#pragma once

#include "core/ObjectEnums.h"
#include "game/ai/AiSettings.h"

namespace cocos2d
{
	class Vec2;
}

namespace game
{
class Tank;

/**
 * A stand-in for the human player, used by the automatic level tests. Every frame it drives the player's tank
 * through the same commands a human would give: it heads for the nearest enemy, lines up with it, fires,
 * and uses its skills when enemies are close. Out of shells it switches to a gun that needs none, and back when
 * shells are picked up.
 */
class PlayerBot
{
public:
	PlayerBot();

	void Update(float InDelta);
	bool HasUsedFreeWeapon() const;
	ECommandType GetLastMove() const;
	const PlayerBotSettings& GetSettings() const;
	void SetSettings(const PlayerBotSettings& InSettings);

private:
	void Think();
	void ChooseWeapon(Tank& InPlayer);
	void ShootWhenLinedUp(Tank& InPlayer);
	ECommandType ChooseMoveTowards(const Tank& InPlayer, const cocos2d::Vec2& InDelta) const;
	ECommandType ChooseMoveWithoutEnemies(const Tank& InPlayer) const;
	void GiveMove(Tank& InPlayer, ECommandType InMove);

	PlayerBotSettings Settings;
	float ThinkTimer = 0.0f;
	float BlockedTimer = 0.0f;
	bool bUsedFreeWeapon = false;
	ECommandType LastMove = ECommandType::Undefined;
};
} // namespace game

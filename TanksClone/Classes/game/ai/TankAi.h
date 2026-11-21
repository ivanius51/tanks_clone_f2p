#pragma once

#include "cocos2d.h"

#include "core/ObjectEnums.h"
#include "game/ai/AiSettings.h"
#include "model/AiFeatureSet.h"
#include "model/CampWatch.h"

namespace tanks
{

class Tank;

/**
 * Drives a tank that is not controlled by the player, only through the tank's public commands, like a human would.
 * Turns across its way of travel wait until the tank reaches the middle of a cell (or is blocked),
 * so it keeps to the grid without being snapped to it. How it behaves comes from its AiType profile in the config.
 */
class TankAi
{
public:
	TankAi(Tank& InOwner, AiType InType);

	void Update(float InDelta);
	// The tank could not move: pick another way on the next update.
	void OnBlocked();

	AiType GetType() const;
	const TankAiSettings& GetSettings() const;
	void SetSettings(const TankAiSettings& InSettings);
	// Difficulty: decisions come InThinkScale times as often apart, random fire InFireRateScale times as often,
	// and the target may be InAimScale times as far off line.
	void ScaleBehaviour(float InThinkScale, float InFireRateScale, float InAimScale);
	// Switches behaviours on or off (enemy config, difficulty, level; the last call wins).
	void ApplyFeatures(const model::AiSwitches& InSwitches);
	bool HasFeature(model::AiFeature InFeature) const;
	// The player has stayed in one spot long enough to be hunted down (anti_camp).
	bool IsPlayerCamping() const;
	CommandType GetLastCommand() const;
	CommandType GetPendingCommand() const;

private:
	void ChooseDirection();
	void TryToShoot(float InDelta);
	// Gives the waiting command once the tank may turn.
	void ApplyPendingCommand(float InDelta);
	bool IsNearCellCenter(float InDelta) const;
	bool IsMovingVertically() const;
	void TryToDodge(float InDelta);
	// The player's position, led for the travel time of this tank's shot when the profile says so.
	cocos2d::Vec2 AimPoint(const Tank& InPlayer) const;
	float GetHealthShare() const;
	// While reloading, step out of the player's row or column.
	void TryToTakeCover();
	// Swaps InCommand for a turn aside now and InCommand a moment later.
	CommandType FakeTurn(CommandType InCommand);
	void UpdateFakeTurn(float InDelta);
	void ForceCommand(CommandType InCommand);
	// A move towards a pickup nearby to keep the player off it; Undefined when there is none.
	CommandType GuardDropMove() const;
	// In a group only the tank nearest the player comes head on, the rest flank.
	bool IsGroupPinner(const cocos2d::Vec2& InPlayerPosition) const;

	Tank& Owner;
	AiType Type;
	TankAiSettings Settings;
	float ThinkTimer = 0.0f;
	CommandType LastCommand = CommandType::Undefined;
	CommandType PendingCommand = CommandType::Undefined;
	float PendingTime = 0.0f;
	float DodgeTimer = 0.0f;
	model::AiFeatureSet Features;
	model::CampWatch Camp;
	float FakeTurnTimer = 0.0f;
	CommandType FakeTurnReturn = CommandType::Undefined;
};

} // namespace tanks

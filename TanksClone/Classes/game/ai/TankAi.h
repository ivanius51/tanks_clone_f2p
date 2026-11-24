#pragma once

#include "cocos2d.h"

#include "core/ObjectEnums.h"
#include "game/ai/AiSettings.h"
#include "model/AiFeatureSet.h"
#include "model/AiTactic.h"
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
	void OnBlocked();

	AiType GetType() const;
	const TankAiSettings& GetSettings() const;
	void SetSettings(const TankAiSettings& InSettings);
	void ScaleBehaviour(float InThinkScale, float InFireRateScale, float InAimScale);
	void ApplyFeatures(const model::AiSwitches& InSwitches);
	bool HasFeature(model::AiFeature InFeature) const;
	bool IsPlayerCamping() const;
	model::AiTactic GetTactic() const;
	void ForceTactic(model::AiTactic InTactic);
	CommandType GetLastCommand() const;
	CommandType GetPendingCommand() const;

private:
	void ChooseDirection();
	CommandType HeadFor(const cocos2d::Vec2& InTarget) const;
	void TryToShoot(float InDelta);
	void ApplyPendingCommand(float InDelta);
	bool IsNearCellCenter(float InDelta) const;
	bool IsMovingVertically() const;
	void TryToDodge(float InDelta);
	cocos2d::Vec2 AimPoint(const Tank& InPlayer) const;
	float GetHealthShare() const;
	void TryToTakeCover();
	CommandType FakeTurn(CommandType InCommand);
	void UpdateFakeTurn(float InDelta);
	void ForceCommand(CommandType InCommand);
	CommandType GuardDropMove() const;
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
	model::AiTactic Tactic = model::AiTactic::Wander;
	model::AiTactic FakeTurnTactic = model::AiTactic::Wander;
};
} // namespace tanks

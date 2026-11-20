#pragma once

#include "core/ObjectEnums.h"
#include "game/ai/AiSettings.h"

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
	CommandType GetLastCommand() const;
	CommandType GetPendingCommand() const;

private:
	void ChooseDirection();
	void TryToShoot(float InDelta);
	// Gives the waiting command once the tank may turn.
	void ApplyPendingCommand(float InDelta);
	bool IsNearCellCenter(float InDelta) const;
	bool IsMovingVertically() const;

	Tank& Owner;
	AiType Type;
	TankAiSettings Settings;
	float ThinkTimer = 0.0f;
	CommandType LastCommand = CommandType::Undefined;
	CommandType PendingCommand = CommandType::Undefined;
	float PendingTime = 0.0f;
};

} // namespace tanks

#pragma once

namespace tanks
{

// Loads everything the game needs before the first screen: configs, catalogs, the shared sprite atlases,
// profile, sounds. The atlases of a level are loaded by the level itself.
// In test mode the player's profile on disk is not touched (a fresh one is used) and sound is off.
void LoadGameData(bool bInTestMode);

} // namespace tanks

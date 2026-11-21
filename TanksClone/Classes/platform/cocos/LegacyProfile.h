#pragma once

#include "ports/IStorage.h"

namespace tanks::view::cocos
{

/**
 * Older versions kept the profile in "profile.plist". Converts it once to "profile.json" (same keys) so the
 * progress carries over; the old file stays. Returns true when it converted something.
 */
bool MigrateLegacyProfile(ports::IStorage& InStorage);

} // namespace tanks::view::cocos

#pragma once

/**
 * Precompiled header of the Cocos game target, force-included into every .cpp
 * except the engine-free model/ files. It only speeds up the build: each file
 * still includes what it uses, so nothing may rely on this list.
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "cocos2d.h"
#include "2d/CCFontAtlas.h"
#include "2d/CCFontAtlasCache.h"
#include "base/CCRefPtr.h"

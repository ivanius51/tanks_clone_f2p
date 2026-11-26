#pragma once

#include <string>
#include <string_view>
#include "templates/TContainers.h"

namespace cocos2d { class Node; }

namespace game
{
namespace tests
{

/** One entry of configs/test/galleries.json: an id, a string id for its menu button and its own config. */
struct GalleryEntry
{
	std::string Id;
	std::string NameKey;
	std::string Config;
};

/**
 * Debug galleries: a test level with tanks or effects laid out in a grid so their look can be checked at a glance.
 * Each gallery config names its level and fills the grid with one or more of:
 * "states" (one enemy type frozen in look states), "all_enemies" (one tank of every type) and
 * "effects" (effects from configs/views/effects.json replayed in a loop, each from the four sides).
 */
class Galleries
{
public:
	static TArray<GalleryEntry> GetAll();
	// Opens the gallery with this id; does nothing for an unknown id.
	static void Play(std::string_view InId);
	// The gallery list as an overlay on this scene: picking one opens it.
	static void ShowPicker(cocos2d::Node& InScene, int InZOrder);
};

} // namespace tests
} // namespace game

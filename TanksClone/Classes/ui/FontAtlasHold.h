#pragma once


#include "2d/CCFontAtlas.h"
#include "base/CCRefPtr.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{
/**
 * Keeps font atlases alive while the engine empties its font cache. The engine purge releases a live label's atlas
 * from inside that atlas's own purgeTexturesAtlas(), so without a hold the atlas can be deleted mid-call.
 * Atlases are engine Ref objects with an intrusive counter, so they are held by RefPtr, not std smart pointers.
 */
class FontAtlasHold
{
public:
	void Add(cocos2d::FontAtlas* InAtlas);

private:
	TArray<cocos2d::RefPtr<cocos2d::FontAtlas>> Atlases;
};
} // namespace game::view::cocos

#include "ui/FontAtlasHold.h"

#include <algorithm>

namespace game::view::cocos
{

void FontAtlasHold::Add(cocos2d::FontAtlas* const InAtlas)
{
	const bool bHeld = std::any_of(Atlases.begin(), Atlases.end(), [InAtlas](const cocos2d::RefPtr<cocos2d::FontAtlas>& InHeld)
	{
		return InHeld.get() == InAtlas;
	});
	if ( InAtlas && !bHeld )
	{
		Atlases.EmplaceBack(InAtlas);
	}
}

} // namespace game::view::cocos

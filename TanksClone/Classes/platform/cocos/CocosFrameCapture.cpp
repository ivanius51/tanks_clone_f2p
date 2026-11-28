#include "platform/cocos/CocosFrameCapture.h"

#include <cstring>

#include "cocos2d.h"

#include "base/ccUtils.h"

namespace game::view::cocos
{
bool CocosFrameCapture::Capture(cocos2d::Node* const InRoot, TArray<std::uint8_t>& OutRgba, int& OutWidth, int& OutHeight)
{
	if ( !InRoot )
	{
		return false;
	}
	cocos2d::Image* const Picture = cocos2d::utils::captureNode(InRoot);
	if ( !Picture || !Picture->getData() )
	{
		return false;
	}
	OutWidth = Picture->getWidth();
	OutHeight = Picture->getHeight();
	const size_t Bytes = static_cast<size_t>(OutWidth) * static_cast<size_t>(OutHeight) * 4;
	OutRgba.Resize(Bytes);
	std::memcpy(OutRgba.GetData(), Picture->getData(), Bytes);
	Picture->release();
	return true;
}
} // namespace game::view::cocos

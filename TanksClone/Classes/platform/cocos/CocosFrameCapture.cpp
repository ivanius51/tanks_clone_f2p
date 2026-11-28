#include "platform/cocos/CocosFrameCapture.h"

#include <cstring>

#include "cocos2d.h"

#include "base/ccUtils.h"

namespace game::view::cocos
{
namespace
{
	constexpr float PixelRoundingMargin = 0.25f;
}

bool CocosFrameCapture::Capture(cocos2d::Node* const InRoot, TArray<std::uint8_t>& OutRgba, int& OutWidth, int& OutHeight)
{
	if ( !InRoot )
	{
		return false;
	}
	return CopyPixels(cocos2d::utils::captureNode(InRoot), OutRgba, OutWidth, OutHeight);
}

bool CocosFrameCapture::CaptureScreen(cocos2d::Node* const InRoot, const int InWidth, const int InHeight, TArray<std::uint8_t>& OutRgba, int& OutWidth, int& OutHeight)
{
	if ( !InRoot || InWidth <= 0 || InHeight <= 0 )
	{
		return false;
	}
	// The engine sizes a render texture in points and multiplies by the content scale; the margin keeps the truncation from losing a pixel.
	const float ContentScale = cocos2d::Director::getInstance()->getContentScaleFactor();
	cocos2d::RenderTexture* const Target = cocos2d::RenderTexture::create((InWidth + PixelRoundingMargin) / ContentScale, (InHeight + PixelRoundingMargin) / ContentScale,
		cocos2d::Texture2D::PixelFormat::RGBA8888, GL_DEPTH24_STENCIL8);
	if ( !Target )
	{
		return false;
	}
	// Keeping the screen projection while the viewport is the texture stretches the whole screen over it.
	Target->setKeepMatrix(true);
	Target->beginWithClear(0.0f, 0.0f, 0.0f, 1.0f);
	InRoot->visit();
	Target->end();
	cocos2d::Director::getInstance()->getRenderer()->render();
	return CopyPixels(Target->newImage(), OutRgba, OutWidth, OutHeight);
}

bool CocosFrameCapture::CopyPixels(cocos2d::Image* const Picture, TArray<std::uint8_t>& OutRgba, int& OutWidth, int& OutHeight)
{
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

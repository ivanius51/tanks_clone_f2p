#pragma once

#include "cocos2d.h"

#include "model/MathTypes.h"

namespace game::view::cocos
{
inline cocos2d::Vec2 ToCocos(const model::Vec2f& InVector) { return { InVector.X, InVector.Y }; }
inline model::Vec2f FromCocos(const cocos2d::Vec2& InVector) { return { InVector.x, InVector.y }; }

inline cocos2d::Size ToCocosSize(const model::Vec2f& InSize) { return { InSize.X, InSize.Y }; }
inline model::Vec2f FromCocos(const cocos2d::Size& InSize) { return { InSize.width, InSize.height }; }

inline cocos2d::Rect ToCocos(const model::RectF& InRect)
{
	return { InRect.Origin.X, InRect.Origin.Y, InRect.Size.X, InRect.Size.Y };
}
inline model::RectF FromCocos(const cocos2d::Rect& InRect)
{
	return { InRect.origin.x, InRect.origin.y, InRect.size.width, InRect.size.height };
}

inline cocos2d::Color4B ToCocos(const model::Color& InColor) { return { InColor.R, InColor.G, InColor.B, InColor.A }; }
inline cocos2d::Color3B ToCocos3B(const model::Color& InColor) { return { InColor.R, InColor.G, InColor.B }; }
inline model::Color FromCocos(const cocos2d::Color4B& InColor) { return { InColor.r, InColor.g, InColor.b, InColor.a }; }
inline model::Color FromCocos(const cocos2d::Color3B& InColor) { return { InColor.r, InColor.g, InColor.b }; }
} // namespace game::view::cocos

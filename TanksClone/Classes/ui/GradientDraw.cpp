#include "ui/GradientDraw.h"

#include <algorithm>
#include <cmath>

#include "ui/CocosConvert.h"

namespace game::view::cocos
{

namespace
{
	constexpr float StripThickness = 3.0f;
	constexpr int MaxStrips = 200;

	// How far a strip is cut in from the side by a rounded corner, InDistance from the near edge.
	float CornerInset(const float InDistance, const float InRadius)
	{
		if ( InRadius <= 0.0f || InDistance >= InRadius )
		{
			return 0.0f;
		}
		const float Rise = InRadius - InDistance;
		return InRadius - std::sqrt(std::max(InRadius * InRadius - Rise * Rise, 0.0f));
	}
}

void FillGradientRect(cocos2d::DrawNode* const InNode, const cocos2d::Size& InHalf, const float InRadius, const model::Gradient& InGradient)
{
	if ( !InGradient.IsSet() || InHalf.width <= 0.0f || InHalf.height <= 0.0f )
	{
		return;
	}
	const bool bVertical = InGradient.Direction == model::GradientDirection::Vertical;
	const float Along = bVertical ? InHalf.height : InHalf.width;
	const float Across = bVertical ? InHalf.width : InHalf.height;
	const float Radius = std::clamp(InRadius, 0.0f, std::min(InHalf.width, InHalf.height));
	const int Strips = std::clamp(static_cast<int>(std::ceil(Along * 2.0f / StripThickness)), 1, MaxStrips);
	const float Step = Along * 2.0f / static_cast<float>(Strips);
	for ( int Index = 0; Index < Strips; ++Index )
	{
		const float Start = static_cast<float>(Index) * Step;
		const float Middle = Start + Step / 2.0f;
		const float Inset = std::max(CornerInset(Middle, Radius), CornerInset(Along * 2.0f - Middle, Radius));
		const cocos2d::Color4F Color(ToCocos(InGradient.Sample(Middle / (Along * 2.0f))));
		// The overlap of one unit hides hairlines between neighbouring strips.
		const float Near = Start;
		const float Far = std::min(Start + Step + 1.0f, Along * 2.0f);
		if ( bVertical )
		{
			InNode->drawSolidRect(cocos2d::Vec2(-Across + Inset, Along - Far), cocos2d::Vec2(Across - Inset, Along - Near), Color);
		}
		else
		{
			InNode->drawSolidRect(cocos2d::Vec2(-Along + Near, -Across + Inset), cocos2d::Vec2(-Along + Far, Across - Inset), Color);
		}
	}
}

} // namespace game::view::cocos

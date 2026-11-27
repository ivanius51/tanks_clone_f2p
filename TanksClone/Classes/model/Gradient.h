#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

#include "model/MathTypes.h"

namespace game::model
{

constexpr int MaxGradientStops = 5;

enum class GradientDirection
{
	Vertical,
	Horizontal
};

inline bool EnumFromName(const std::string_view InName, GradientDirection& OutDirection)
{
	if ( InName == "vertical" ) { OutDirection = GradientDirection::Vertical; return true; }
	if ( InName == "horizontal" ) { OutDirection = GradientDirection::Horizontal; return true; }
	return false;
}

/**
 * A transition between up to MaxGradientStops colors spread evenly along a direction. Vertical runs from the top
 * (first stop) to the bottom (last stop), horizontal from the left to the right. Fewer than two stops is no gradient.
 */
struct Gradient
{
	GradientDirection Direction = GradientDirection::Vertical;
	std::array<Color, MaxGradientStops> Stops;
	int Count = 0;

	bool IsSet() const { return Count >= 2; }

	bool AddStop(const Color& InColor)
	{
		if ( Count >= MaxGradientStops )
		{
			return false;
		}
		Stops[static_cast<std::size_t>(Count++)] = InColor;
		return true;
	}

	// InPosition 0 is the first stop, 1 the last; outside is clamped.
	Color Sample(const float InPosition) const
	{
		if ( Count <= 0 )
		{
			return Color();
		}
		if ( Count == 1 )
		{
			return Stops[0];
		}
		const float Scaled = std::clamp(InPosition, 0.0f, 1.0f) * static_cast<float>(Count - 1);
		const int Low = std::min(static_cast<int>(Scaled), Count - 2);
		const float Blend = Scaled - static_cast<float>(Low);
		const Color& A = Stops[static_cast<std::size_t>(Low)];
		const Color& B = Stops[static_cast<std::size_t>(Low + 1)];
		const auto Mix = [Blend](const std::uint8_t InA, const std::uint8_t InB)
		{
			return static_cast<std::uint8_t>(std::lround(static_cast<float>(InA) + (static_cast<float>(InB) - static_cast<float>(InA)) * Blend));
		};
		return Color(Mix(A.R, B.R), Mix(A.G, B.G), Mix(A.B, B.B), Mix(A.A, B.A));
	}

	// Every stop multiplied by InFactor (above 1 lightens, below 1 darkens); alpha is kept.
	Gradient Scaled(const float InFactor) const
	{
		Gradient Result = *this;
		for ( Color& Stop : Result.Stops )
		{
			const auto Mul = [InFactor](const std::uint8_t InChannel)
			{
				return static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(InChannel) * InFactor), 0L, 255L));
			};
			Stop = Color(Mul(Stop.R), Mul(Stop.G), Mul(Stop.B), Stop.A);
		}
		return Result;
	}
};

// A gradient of one color, for a flat fill that is handled like the others.
inline Gradient FlatGradient(const Color& InColor)
{
	Gradient Result;
	Result.AddStop(InColor);
	Result.AddStop(InColor);
	return Result;
}

} // namespace game::model

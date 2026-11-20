#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tanks::model
{

/** A 2D vector of the engine-free model: positions, sizes and velocities in world units. */
struct Vec2f
{
	float X = 0.0f;
	float Y = 0.0f;

	constexpr Vec2f() = default;
	constexpr Vec2f(const float InX, const float InY) : X(InX), Y(InY) {}

	constexpr Vec2f operator+(const Vec2f& InOther) const { return { X + InOther.X, Y + InOther.Y }; }
	constexpr Vec2f operator-(const Vec2f& InOther) const { return { X - InOther.X, Y - InOther.Y }; }
	constexpr Vec2f operator*(const float InScale) const { return { X * InScale, Y * InScale }; }
	constexpr Vec2f operator/(const float InScale) const { return { X / InScale, Y / InScale }; }
	constexpr Vec2f operator-() const { return { -X, -Y }; }
	constexpr Vec2f& operator+=(const Vec2f& InOther) { X += InOther.X; Y += InOther.Y; return *this; }
	constexpr Vec2f& operator-=(const Vec2f& InOther) { X -= InOther.X; Y -= InOther.Y; return *this; }
	constexpr Vec2f& operator*=(const float InScale) { X *= InScale; Y *= InScale; return *this; }
	constexpr bool operator==(const Vec2f& InOther) const { return X == InOther.X && Y == InOther.Y; }
	constexpr bool operator!=(const Vec2f& InOther) const { return !(*this == InOther); }

	constexpr float Dot(const Vec2f& InOther) const { return X * InOther.X + Y * InOther.Y; }
	constexpr float LengthSquared() const { return Dot(*this); }
	float Length() const { return std::sqrt(LengthSquared()); }
	float DistanceTo(const Vec2f& InOther) const { return (*this - InOther).Length(); }

	// The zero vector stays zero.
	Vec2f Normalized() const
	{
		const float CurrentLength = Length();
		return CurrentLength > 0.0f ? *this / CurrentLength : Vec2f();
	}

	bool IsNear(const Vec2f& InOther, const float InTolerance) const
	{
		return std::fabs(X - InOther.X) <= InTolerance && std::fabs(Y - InOther.Y) <= InTolerance;
	}
};

constexpr Vec2f operator*(const float InScale, const Vec2f& InVector)
{
	return InVector * InScale;
}

/** An axis-aligned rectangle: Origin is the bottom-left corner, Y goes up (as in the world). */
struct RectF
{
	Vec2f Origin;
	Vec2f Size;

	constexpr RectF() = default;
	constexpr RectF(const Vec2f& InOrigin, const Vec2f& InSize) : Origin(InOrigin), Size(InSize) {}
	constexpr RectF(const float InX, const float InY, const float InWidth, const float InHeight)
		: Origin(InX, InY), Size(InWidth, InHeight) {}

	static constexpr RectF FromCenter(const Vec2f& InCenter, const Vec2f& InSize)
	{
		return { InCenter - InSize / 2.0f, InSize };
	}

	constexpr float GetMinX() const { return Origin.X; }
	constexpr float GetMinY() const { return Origin.Y; }
	constexpr float GetMaxX() const { return Origin.X + Size.X; }
	constexpr float GetMaxY() const { return Origin.Y + Size.Y; }
	constexpr Vec2f GetCenter() const { return Origin + Size / 2.0f; }

	// Edges are inclusive, like cocos2d::Rect::containsPoint.
	constexpr bool Contains(const Vec2f& InPoint) const
	{
		return InPoint.X >= GetMinX() && InPoint.X <= GetMaxX() && InPoint.Y >= GetMinY() && InPoint.Y <= GetMaxY();
	}

	// Touching edges do not intersect.
	constexpr bool Intersects(const RectF& InOther) const
	{
		return GetMinX() < InOther.GetMaxX() && InOther.GetMinX() < GetMaxX()
			&& GetMinY() < InOther.GetMaxY() && InOther.GetMinY() < GetMaxY();
	}

	constexpr bool operator==(const RectF& InOther) const { return Origin == InOther.Origin && Size == InOther.Size; }
	constexpr bool operator!=(const RectF& InOther) const { return !(*this == InOther); }
};

/** An RGBA color, 0-255 per channel. */
struct Color
{
	std::uint8_t R = 255;
	std::uint8_t G = 255;
	std::uint8_t B = 255;
	std::uint8_t A = 255;

	constexpr Color() = default;
	constexpr Color(const std::uint8_t InR, const std::uint8_t InG, const std::uint8_t InB, const std::uint8_t InA = 255)
		: R(InR), G(InG), B(InB), A(InA) {}

	constexpr bool operator==(const Color& InOther) const
	{
		return R == InOther.R && G == InOther.G && B == InOther.B && A == InOther.A;
	}
	constexpr bool operator!=(const Color& InOther) const { return !(*this == InOther); }
};

} // namespace tanks::model

#pragma once

#include "model/MathTypes.h"

namespace game::model
{
enum class ShotSide
{
	Front,
	Side,
	Back
};

/**
 * Which side of a vehicle a shot hits: from the unit vector it faces and the direction the shot flies.
 * A shot flying against the facing hits the front, one flying along it hits the back.
 */
enum class HitFace
{
	Front,
	Back,
	Left,
	Right
};

class ShotSideClassifier
{
public:
	static ShotSide Classify(const Vec2f& InFacing, const Vec2f& InShotDirection);
	static HitFace ClassifyFace(const Vec2f& InFacing, const Vec2f& InShotDirection);
};
} // namespace game::model

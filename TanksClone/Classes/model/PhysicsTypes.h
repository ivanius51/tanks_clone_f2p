#pragma once

#include "model/MathTypes.h"

namespace game::model
{
/** Identifies a body inside a physics world; 0 means no body. */
struct BodyId
{
	unsigned Value = 0;

	bool IsValid() const { return Value != 0; }
	bool operator==(const BodyId& InOther) const { return Value == InOther.Value; }
	bool operator!=(const BodyId& InOther) const { return Value != InOther.Value; }
};

/** Static bodies never move, dynamic ones are moved by the simulation, kinematic ones are moved by the caller. */
enum class BodyType
{
	Static,
	Dynamic,
	Kinematic
};

/** The standard collision shapes. A grid is a box made of cells that can be carved away. */
enum class ShapeKind
{
	Circle,
	Box,
	RoundedBox,
	Grid
};

/** A collision shape. Size is the full width and height; Radius is the circle radius or the corner radius of a rounded box. */
struct Shape
{
	ShapeKind Kind = ShapeKind::Box;
	Vec2f Size = Vec2f(1.0f, 1.0f);
	float Radius = 0.0f;
	// The point of the shape placed at the body position, as a share of its size: (0.5, 0.5) is the center, (0, 0) the bottom left.
	Vec2f Anchor = Vec2f(0.5f, 0.5f);
	int GridCellsPerSide = 0;
};

/** Everything needed to create a body. */
struct BodyDef
{
	BodyType Type = BodyType::Static;
	Shape BodyShape;
	Vec2f Position;
	float RotationDegrees = 0.0f;
	// A sensor reports contacts but does not block.
	bool bIsSensor = false;
};

/** How two bodies met: they started touching, stopped touching, or one was stopped by the other this step. */
enum class ContactKind
{
	Begin,
	End,
	Blocked
};
} // namespace game::model

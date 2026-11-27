#pragma once

#include <array>

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
	int GridCellHealth = 1;
};

/** The kind of object a body is, as other bodies see it (the object type of an Unreal collision profile). */
enum class CollisionChannel
{
	WorldStatic,
	WorldDynamic,
	Pawn,
	Projectile,
	Count
};

constexpr int CollisionChannelCount = static_cast<int>(CollisionChannel::Count);

/** What a body does when it meets a channel: pass through, report an overlap, or stop. Ordered so that the weaker of two answers wins. */
enum class CollisionResponse
{
	Ignore,
	Overlap,
	Block
};

/** The channel a body belongs to and its response to every channel. */
struct CollisionProfile
{
	CollisionChannel ObjectType = CollisionChannel::WorldStatic;
	std::array<CollisionResponse, CollisionChannelCount> Responses = { CollisionResponse::Block, CollisionResponse::Block, CollisionResponse::Block, CollisionResponse::Block };

	CollisionResponse GetResponseTo(const CollisionChannel InChannel) const { return Responses[static_cast<size_t>(InChannel)]; }
	void SetResponseTo(const CollisionChannel InChannel, const CollisionResponse InResponse) { Responses[static_cast<size_t>(InChannel)] = InResponse; }
};

/** Everything needed to create a body. UserTag is returned in events so the game can tell what the body stands for. */
struct BodyDef
{
	BodyType Type = BodyType::Static;
	Shape BodyShape;
	CollisionProfile Profile;
	Vec2f Position;
	float RotationDegrees = 0.0f;
	unsigned UserTag = 0;
};

/** The answer of a sweep or the detail of a hit (the fields of Unreal's FHitResult that a 2D world needs). */
struct HitResult
{
	bool bBlockingHit = false;
	bool bStartPenetrating = false;
	// 0-1 share of the move completed before the hit.
	float Time = 1.0f;
	float Distance = 0.0f;
	// Where the moving shape's center was at the hit, the point on the other body, and its surface normal there.
	Vec2f Location;
	Vec2f ImpactPoint;
	Vec2f Normal;
	BodyId Item;
};

/** What happened between two bodies: a blocking hit, or an overlap that began or ended. */
enum class ContactKind
{
	Hit,
	OverlapBegin,
	OverlapEnd
};
} // namespace game::model

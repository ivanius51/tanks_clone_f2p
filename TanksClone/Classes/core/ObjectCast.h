#pragma once

#include <cassert>
#include <type_traits>

#include "core/ObjectEnums.h"

namespace tanks
{
class GameObject;
class PhysicsObject;
class MovableObject;
class Tank;
class Bullet;
class Wall;
class Bonus;

/**
 * Which ObjectType values a GameObject class stands for. Casting to a class without a specialization does not compile.
 * GetType() is the type to search the factory by (Undefined = every type).
 */
template <typename TObject>
struct ObjectClass;

template <>
struct ObjectClass<GameObject>
{
	static constexpr ObjectType GetType() { return ObjectType::Undefined; }
	static constexpr bool Matches(ObjectType) { return true; }
};

template <>
struct ObjectClass<PhysicsObject>
{
	static constexpr ObjectType GetType() { return ObjectType::Undefined; }
	static constexpr bool Matches(const ObjectType InType)
	{
		return InType == ObjectType::Vehicle || InType == ObjectType::Bullet || InType == ObjectType::Wall || InType == ObjectType::Bonus;
	}
};

template <>
struct ObjectClass<MovableObject>
{
	static constexpr ObjectType GetType() { return ObjectType::Vehicle; }
	static constexpr bool Matches(const ObjectType InType) { return InType == ObjectType::Vehicle; }
};

template <>
struct ObjectClass<Tank>
{
	static constexpr ObjectType GetType() { return ObjectType::Vehicle; }
	static constexpr bool Matches(const ObjectType InType) { return InType == ObjectType::Vehicle; }
};

template <>
struct ObjectClass<Bullet>
{
	static constexpr ObjectType GetType() { return ObjectType::Bullet; }
	static constexpr bool Matches(const ObjectType InType) { return InType == ObjectType::Bullet; }
};

template <>
struct ObjectClass<Wall>
{
	static constexpr ObjectType GetType() { return ObjectType::Wall; }
	static constexpr bool Matches(const ObjectType InType) { return InType == ObjectType::Wall; }
};

template <>
struct ObjectClass<Bonus>
{
	static constexpr ObjectType GetType() { return ObjectType::Bonus; }
	static constexpr bool Matches(const ObjectType InType) { return InType == ObjectType::Bonus; }
};

/**
 * static_cast for types that are only known at run time and have no type tag (engine nodes, actions).
 * The caller must know the type; Debug builds check it.
 */
template <typename TTarget, typename TSource>
TTarget* CheckedCast(TSource* const InSource)
{
	static_assert(std::is_base_of<TSource, TTarget>::value, "CheckedCast only casts down a class hierarchy");
	assert(InSource == nullptr || dynamic_cast<TTarget*>(InSource) != nullptr);
	return static_cast<TTarget*>(InSource);
}

template <typename TTarget, typename TSource>
const TTarget* CheckedCast(const TSource* const InSource)
{
	return CheckedCast<TTarget>(const_cast<TSource*>(InSource));
}

/**
 * Casts a game object down by its ObjectType tag: nullptr when the object is of another type.
 * The tag check happens in every build; Debug builds also check that the tag tells the truth.
 */
template <typename TTarget, typename TSource>
TTarget* ObjectCast(TSource* const InObject)
{
	static_assert(std::is_base_of<GameObject, TSource>::value, "ObjectCast works on game objects only");
	static_assert(std::is_base_of<TSource, TTarget>::value, "ObjectCast only casts down; casting up needs no cast");
	if ( InObject == nullptr || !ObjectClass<TTarget>::Matches(InObject->GetType()) )
	{
		return nullptr;
	}
	return CheckedCast<TTarget>(InObject);
}

template <typename TTarget, typename TSource>
const TTarget* ObjectCast(const TSource* const InObject)
{
	return ObjectCast<TTarget>(const_cast<TSource*>(InObject));
}
} // namespace tanks

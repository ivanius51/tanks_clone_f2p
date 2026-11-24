#pragma once

#include <cstddef>
#include <type_traits>

namespace tanks
{
/** A compile-time list of types: TypeIndexOf gives a type's position, IsInTypeList checks membership. */
template <typename... TTypes>
struct TypeList
{
	static constexpr std::size_t Size = sizeof...(TTypes);
};

template <typename TType, typename TList>
struct IsInTypeList;

template <typename TType, typename... TTypes>
struct IsInTypeList<TType, TypeList<TTypes...>> : std::bool_constant<(std::is_same_v<TType, TTypes> || ...)>
{
};

template <typename TType, typename TList>
struct TypeIndexOf;

template <typename TType, typename... TRest>
struct TypeIndexOf<TType, TypeList<TType, TRest...>> : std::integral_constant<std::size_t, 0>
{
};

template <typename TType, typename TFirst, typename... TRest>
struct TypeIndexOf<TType, TypeList<TFirst, TRest...>> : std::integral_constant<std::size_t, 1 + TypeIndexOf<TType, TypeList<TRest...>>::value>
{
};
} // namespace tanks

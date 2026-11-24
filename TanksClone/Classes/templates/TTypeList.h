#pragma once

#include <cstddef>
#include <type_traits>

namespace tanks
{
/** A compile-time list of types: TypeIndexOf gives a type's position, IsInTypeList checks membership. */
template <typename... TTypes>
struct TTypeList
{
	static constexpr std::size_t Size = sizeof...(TTypes);
};

template <typename TType, typename TList>
struct IsInTypeList;

template <typename TType, typename... TTypes>
struct IsInTypeList<TType, TTypeList<TTypes...>> : std::bool_constant<(std::is_same_v<TType, TTypes> || ...)>
{
};

template <typename TType, typename TList>
struct TypeIndexOf;

template <typename TType, typename... TRest>
struct TypeIndexOf<TType, TTypeList<TType, TRest...>> : std::integral_constant<std::size_t, 0>
{
};

template <typename TType, typename TFirst, typename... TRest>
struct TypeIndexOf<TType, TTypeList<TFirst, TRest...>> : std::integral_constant<std::size_t, 1 + TypeIndexOf<TType, TTypeList<TRest...>>::value>
{
};
} // namespace tanks

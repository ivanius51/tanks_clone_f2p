#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace game
{
template <typename T>
struct TTypeTag
{
	using Type = T;
};

template <typename... TTypes>
struct TTypeList
{
	static constexpr std::size_t Size = sizeof...(TTypes);
	static constexpr bool bIsEmpty = (Size == 0);

	template <typename TType>
	static constexpr bool Contains = (std::is_same_v<TType, TTypes> || ...);

	template <typename TType>
	static constexpr std::size_t CountOf = ((std::is_same_v<TType, TTypes> ? 1 : 0) + ... + 0);

	static constexpr bool bHasUniqueTypes = ((CountOf<TTypes> == 1) && ...);

	template <std::size_t Index>
	using At = std::tuple_element_t<Index, std::tuple<TTypes...>>;

	template <template <typename...> class TTarget>
	using ApplyTo = TTarget<TTypes...>;

	template <template <typename> class TMetaFn>
	using Transform = TTypeList<TMetaFn<TTypes>...>;

	template <typename TFunc>
	static constexpr void ForEach(TFunc&& InFunc)
	{
		(InFunc(TTypeTag<TTypes>{}), ...);
	}

	template <typename TFunc>
	static constexpr void ForEachReverse(TFunc&& InFunc)
	{
		int Dummy = 0;
		((Dummy = (InFunc(TTypeTag<TTypes>{}), 0)), ...);
		(void)Dummy;
	}

	template <typename TPred>
	static constexpr bool AllOf(TPred&& InPred)
	{
		return (static_cast<bool>(InPred(TTypeTag<TTypes>{})) && ...);
	}

	template <typename TPred>
	static constexpr bool AnyOf(TPred&& InPred)
	{
		return (static_cast<bool>(InPred(TTypeTag<TTypes>{})) || ...);
	}
};

template <typename TType, typename TList>
struct IsInTypeList;

template <typename TType, typename... TTypes>
struct IsInTypeList<TType, TTypeList<TTypes...>> : std::bool_constant<(std::is_same_v<TType, TTypes> || ...)>
{
};

template <typename TType, typename TList>
inline constexpr bool IsInTypeList_v = IsInTypeList<TType, TList>::value;

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

template <typename TType, typename TList>
inline constexpr std::size_t TypeIndexOf_v = TypeIndexOf<TType, TList>::value;

template <typename... TLists>
struct ConcatTypeLists;

template <>
struct ConcatTypeLists<>
{
	using Type = TTypeList<>;
};

template <typename... TTypes>
struct ConcatTypeLists<TTypeList<TTypes...>>
{
	using Type = TTypeList<TTypes...>;
};

template <typename... TFirst, typename... TSecond, typename... TRest>
struct ConcatTypeLists<TTypeList<TFirst...>, TTypeList<TSecond...>, TRest...>
{
	using Type = typename ConcatTypeLists<TTypeList<TFirst..., TSecond...>, TRest...>::Type;
};

template <typename... TLists>
using ConcatTypeLists_t = typename ConcatTypeLists<TLists...>::Type;

template <typename TList, template <typename> class TPred>
struct FilterTypeList;

template <template <typename> class TPred>
struct FilterTypeList<TTypeList<>, TPred>
{
	using Type = TTypeList<>;
};

template <typename TFirst, typename... TRest, template <typename> class TPred>
struct FilterTypeList<TTypeList<TFirst, TRest...>, TPred>
{
	using Tail = typename FilterTypeList<TTypeList<TRest...>, TPred>::Type;
	using Type = std::conditional_t<TPred<TFirst>::value, ConcatTypeLists_t<TTypeList<TFirst>, Tail>, Tail>;
};

template <typename TList, template <typename> class TPred>
using FilterTypeList_t = typename FilterTypeList<TList, TPred>::Type;

template <typename TList, typename... TAdded>
using AppendToTypeList_t = ConcatTypeLists_t<TList, TTypeList<TAdded...>>;

template <typename TList, typename... TAdded>
using PrependToTypeList_t = ConcatTypeLists_t<TTypeList<TAdded...>, TList>;

template <typename TListA, typename TListB>
inline constexpr bool AreTypeListsEqual_v = std::is_same_v<TListA, TListB>;

template <typename TSubList, typename TSuperList>
struct IsSubsetOfTypeList : std::false_type {};

template <typename... TSubTypes, typename TSuperList>
struct IsSubsetOfTypeList<TTypeList<TSubTypes...>, TSuperList>
	: std::bool_constant<(TSuperList::template Contains<TSubTypes> && ...)>
{
};

template <typename TSubList, typename TSuperList>
inline constexpr bool IsSubsetOfTypeList_v = IsSubsetOfTypeList<TSubList, TSuperList>::value;

template <typename TListA, typename TListB>
inline constexpr bool AreTypeListsSameSet_v =
	IsSubsetOfTypeList_v<TListA, TListB> && IsSubsetOfTypeList_v<TListB, TListA>;

template <typename TList, typename TExcludeList>
struct ExcludeTypes;

template <typename... TTypes, typename TExcludeList>
struct ExcludeTypes<TTypeList<TTypes...>, TExcludeList>
{
	using Type = ConcatTypeLists_t<
		std::conditional_t<TExcludeList::template Contains<TTypes>, TTypeList<>, TTypeList<TTypes>>...
	>;
};

template <typename TList, typename TExcludeList>
using ExcludeTypes_t = typename ExcludeTypes<TList, TExcludeList>::Type;

template <typename TList, typename TIncludeList>
struct IncludeTypes;

template <typename... TTypes, typename TIncludeList>
struct IncludeTypes<TTypeList<TTypes...>, TIncludeList>
{
	using Type = ConcatTypeLists_t<
		std::conditional_t<TIncludeList::template Contains<TTypes>, TTypeList<TTypes>, TTypeList<>>...
	>;
};

template <typename TList, typename TIncludeList>
using IncludeTypes_t = typename IncludeTypes<TList, TIncludeList>::Type;

template <typename TList, typename TSeen = TTypeList<>>
struct UniqueTypeList;

template <typename TSeen>
struct UniqueTypeList<TTypeList<>, TSeen>
{
	using Type = TSeen;
};

template <typename TFirst, typename... TRest, typename TSeen>
struct UniqueTypeList<TTypeList<TFirst, TRest...>, TSeen>
{
	using NextSeen = std::conditional_t<
		TSeen::template Contains<TFirst>,
		TSeen,
		AppendToTypeList_t<TSeen, TFirst>
	>;
	using Type = typename UniqueTypeList<TTypeList<TRest...>, NextSeen>::Type;
};

template <typename TList>
using UniqueTypeList_t = typename UniqueTypeList<TList>::Type;

template <typename... TLists>
using MergeTypeLists_t = UniqueTypeList_t<ConcatTypeLists_t<TLists...>>;


} // namespace game
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "model/MathTypes.h"
#include "model/RefCounted.h"
#include "templates/TContainers.h"

namespace game::model
{
class IConfigNode;

namespace config_detail
{
	template <typename T, typename = void>
	struct HasEnumFromName : std::false_type
	{
	};

	template <typename T>
	struct HasEnumFromName<T, std::void_t<decltype(EnumFromName(std::declval<std::string_view>(), std::declval<T&>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct HasFromConfig : std::false_type
	{
	};

	template <typename T>
	struct HasFromConfig<T, std::void_t<decltype(std::declval<T&>().FromConfig(std::declval<const IConfigNode&>()))>>
		: std::true_type
	{
	};

	template <typename T>
	struct IsVector : std::false_type
	{
	};

	template <typename T>
	struct IsVector<TArray<T>> : std::true_type
	{
	};

	template <typename T>
	struct IsPersistentKeyMap : std::false_type
	{
	};

	template <typename T, typename TCompare>
	struct IsPersistentKeyMap<TMap<FPersistentString, T, TCompare>> : std::true_type
	{
	};

	template <typename T>
	struct IsStringMap : std::false_type
	{
	};

	template <typename T>
	struct IsStringMap<std::map<std::string, T>> : std::true_type
	{
	};

	template <typename T>
	struct IsStringMap<std::map<std::string, T, std::less<>>> : std::true_type
	{
	};

	template <typename T, typename TCompare>
	struct IsStringMap<TMap<FPersistentString, T, TCompare>> : std::true_type
	{
	};

	template <typename TMapType>
	void AddEntry(TMapType& OutMap, const std::string& InKey, typename TMapType::mapped_type&& InItem)
	{
		if constexpr ( IsPersistentKeyMap<TMapType>::value )
		{
			OutMap.InsertOrAssign(InKey, std::move(InItem));
		}
		else
		{
			OutMap.emplace(InKey, std::move(InItem));
		}
	}

	template <typename T>
	constexpr bool IsSupported = std::is_same_v<T, bool> || std::is_integral_v<T> || std::is_floating_point_v<T>
		|| std::is_same_v<T, std::string> || std::is_same_v<T, Vec2f> || std::is_same_v<T, Color>
		|| (std::is_enum_v<T> && HasEnumFromName<T>::value) || HasFromConfig<T>::value
		|| IsVector<T>::value || IsStringMap<T>::value;
}

/**
 * Read-only access to one node of a config tree, independent of the file format.
 * Loaders read configs only through this interface; the format (JSON today) lives in its implementation.
 *
 * Usage:
 *   const float Speed = Node.GetValue<float>("speed");
 *   const int Lives = Node.GetValueOr("lives", 3);
 *   EAiType Type; const bool bRead = Node.TryRead("ai", Type);
 *   TArray<WaveDef> Waves = Node.GetValue<TArray<WaveDef>>("waves");
 */
class IConfigNode : public TRefCounted
{
public:
	virtual ~IConfigNode() = default;

	virtual const IConfigNode* FindChild(std::string_view InKey) const = 0;
	virtual TArray<std::string> GetKeys() const = 0;
	virtual std::size_t GetItemCount() const = 0;
	virtual const IConfigNode* GetItem(std::size_t InIndex) const = 0;

	virtual std::optional<bool> ReadBool() const = 0;
	virtual std::optional<double> ReadNumber() const = 0;
	virtual std::optional<std::string> ReadString() const = 0;

	bool Has(std::string_view InKey) const
	{
		return FindChild(InKey) != nullptr;
	}

	template <typename TValue>
	bool ReadSelf(TValue& OutValue) const
	{
		static_assert(config_detail::IsSupported<TValue>, "Unsupported type passed to IConfigNode");

		if constexpr ( std::is_same_v<TValue, bool> )
		{
			return Assign(ReadBool(), OutValue);
		}
		else if constexpr ( std::is_integral_v<TValue> || std::is_floating_point_v<TValue> )
		{
			const std::optional<double> Number = ReadNumber();
			if ( Number )
			{
				OutValue = static_cast<TValue>(*Number);
			}
			return Number.has_value();
		}
		else if constexpr ( std::is_same_v<TValue, std::string> )
		{
			return Assign(ReadString(), OutValue);
		}
		else if constexpr ( std::is_enum_v<TValue> )
		{
			const std::optional<std::string> Name = ReadString();
			return Name && EnumFromName(std::string_view(*Name), OutValue);
		}
		else if constexpr ( std::is_same_v<TValue, Vec2f> )
		{
			Vec2f Result;
			if ( GetItemCount() != 2 || !GetItem(0)->ReadSelf(Result.X) || !GetItem(1)->ReadSelf(Result.Y) )
			{
				return false;
			}
			OutValue = Result;
			return true;
		}
		else if constexpr ( config_detail::HasFromConfig<TValue>::value )
		{
			return OutValue.FromConfig(*this);
		}
		else if constexpr ( config_detail::IsVector<TValue>::value )
		{
			TValue Result;
			Result.Reserve(GetItemCount());
			for ( std::size_t Index = 0; Index < GetItemCount(); ++Index )
			{
				typename TValue::value_type Item{};
				if ( !GetItem(Index)->ReadSelf(Item) )
				{
					return false;
				}
				Result.PushBack(std::move(Item));
			}
			OutValue = std::move(Result);
			return true;
		}
		else if constexpr ( config_detail::IsStringMap<TValue>::value )
		{
			TValue Result;
			for ( const std::string& Key : GetKeys() )
			{
				typename TValue::mapped_type Item{};
				if ( !FindChild(Key)->ReadSelf(Item) )
				{
					return false;
				}
				config_detail::AddEntry(Result, Key, std::move(Item));
			}
			OutValue = std::move(Result);
			return true;
		}
		else if constexpr ( std::is_same_v<TValue, Color> )
		{
			const std::size_t Count = GetItemCount();
			int Channels[4] = { 0, 0, 0, 255 };
			if ( Count < 3 || Count > 4 )
			{
				return false;
			}
			for ( std::size_t Index = 0; Index < Count; ++Index )
			{
				if ( !GetItem(Index)->ReadSelf(Channels[Index]) )
				{
					return false;
				}
			}
			OutValue = Color(ToChannel(Channels[0]), ToChannel(Channels[1]), ToChannel(Channels[2]), ToChannel(Channels[3]));
			return true;
		}
	}

	template <typename TValue>
	bool TryRead(std::string_view InKey, TValue& OutValue) const
	{
		const IConfigNode* const Child = FindChild(InKey);
		return Child && Child->ReadSelf(OutValue);
	}

	template <typename TValue>
	TValue GetValueOr(std::string_view InKey, TValue InDefault) const
	{
		TryRead(InKey, InDefault);
		return InDefault;
	}

	std::string GetValueOr(std::string_view InKey, const char* const InDefault) const
	{
		return GetValueOr<std::string>(InKey, std::string(InDefault));
	}

	template <typename TValue, bool bWarn = true>
	TValue GetValue(std::string_view InKey) const
	{
		TValue Result{};
		if ( !TryRead(InKey, Result) )
		{
			if constexpr ( bWarn )
			{
				ReportMissing(InKey);
			}
			return TValue{};
		}
		return Result;
	}

	int GetInt(std::string_view InKey) const { return GetValue<int>(InKey); }
	float GetFloat(std::string_view InKey) const { return GetValue<float>(InKey); }
	bool GetBool(std::string_view InKey) const { return GetValue<bool>(InKey); }
	std::string GetString(std::string_view InKey) const { return GetValue<std::string>(InKey); }

protected:
	virtual void ReportMissing(std::string_view InKey) const = 0;

private:
	template <typename TValue>
	static bool Assign(const std::optional<TValue>& InValue, TValue& OutValue)
	{
		if ( InValue )
		{
			OutValue = *InValue;
		}
		return InValue.has_value();
	}

	static std::uint8_t ToChannel(const int InValue)
	{
		return static_cast<std::uint8_t>(InValue < 0 ? 0 : (InValue > 255 ? 255 : InValue));
	}
};
} // namespace game::model

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "model/MathTypes.h"

namespace tanks::model
{

class IConfigNode;

namespace config_detail
{
	template <typename T, typename = void>
	struct HasEnumFromName : std::false_type
	{
	};

	// An enum is readable by name when an EnumFromName(std::string_view, T&) overload is visible (ADL).
	template <typename T>
	struct HasEnumFromName<T, std::void_t<decltype(EnumFromName(std::declval<std::string_view>(), std::declval<T&>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct HasFromConfig : std::false_type
	{
	};

	// A custom type reads itself when it has bool FromConfig(const IConfigNode&).
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
	struct IsVector<std::vector<T>> : std::true_type
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
 *   const float Speed = Node.GetValue<float>("speed");          // missing: logged, 0
 *   const int Lives = Node.GetValueOr("lives", 3);              // missing: 3, not logged
 *   AiType Type; const bool bRead = Node.TryRead("ai", Type);   // enums by name
 *   std::vector<WaveDef> Waves = Node.GetValue<std::vector<WaveDef>>("waves"); // WaveDef::FromConfig
 */
class IConfigNode
{
public:
	virtual ~IConfigNode() = default;

	// nullptr when the key is missing or this node is not an object.
	virtual const IConfigNode* FindChild(std::string_view InKey) const = 0;
	// Keys of an object node in a stable order; empty for any other node.
	virtual std::vector<std::string> GetKeys() const = 0;
	// Items of an array node; 0 for any other node.
	virtual std::size_t GetItemCount() const = 0;
	virtual const IConfigNode* GetItem(std::size_t InIndex) const = 0;

	virtual std::optional<bool> ReadBool() const = 0;
	virtual std::optional<double> ReadNumber() const = 0;
	virtual std::optional<std::string> ReadString() const = 0;

	bool Has(std::string_view InKey) const
	{
		return FindChild(InKey) != nullptr;
	}

	// Converts this node itself. False (OutValue untouched) when the node has another type.
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
			Result.reserve(GetItemCount());
			for ( std::size_t Index = 0; Index < GetItemCount(); ++Index )
			{
				typename TValue::value_type Item{};
				if ( !GetItem(Index)->ReadSelf(Item) )
				{
					return false;
				}
				Result.push_back(std::move(Item));
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
				Result.emplace(Key, std::move(Item));
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

	// False (OutValue untouched) when the key is missing or has another type. Never logs.
	template <typename TValue>
	bool TryRead(std::string_view InKey, TValue& OutValue) const
	{
		const IConfigNode* const Child = FindChild(InKey);
		return Child && Child->ReadSelf(OutValue);
	}

	// InDefault when the key is missing or has another type. Never logs.
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

	// A required key: a missing or mistyped value is reported and gives a zero value.
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

	// Short forms of GetValue for the common types.
	int GetInt(std::string_view InKey) const { return GetValue<int>(InKey); }
	float GetFloat(std::string_view InKey) const { return GetValue<float>(InKey); }
	bool GetBool(std::string_view InKey) const { return GetValue<bool>(InKey); }
	std::string GetString(std::string_view InKey) const { return GetValue<std::string>(InKey); }

protected:
	// Called for a required key that is missing or has the wrong type (the implementation logs it).
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

} // namespace tanks::model

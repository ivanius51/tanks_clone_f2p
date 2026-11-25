#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace game::model
{
/**
 * A parsed JSON value without engine types: null, bool, int, double, string, array or object.
 * Typed reads take a fallback that is returned when the key is missing or has another type.
 */
class JsonValue
{
public:
	using Array = std::vector<JsonValue>;
	using Object = std::map<std::string, JsonValue, std::less<>>;

	enum class Type
	{
		Null,
		Bool,
		Int,
		Double,
		String,
		Array,
		Object
	};

	JsonValue() = default;
	explicit JsonValue(bool bInValue);
	explicit JsonValue(int InValue);
	explicit JsonValue(double InValue);
	explicit JsonValue(std::string InValue);
	explicit JsonValue(const char* InValue);
	explicit JsonValue(Array InValue);
	explicit JsonValue(Object InValue);

	static JsonValue Parse(std::string_view InText, bool* bOutOk = nullptr);

	Type GetType() const;
	bool IsNull() const;
	bool IsBool() const;
	bool IsNumber() const;
	bool IsString() const;
	bool IsArray() const;
	bool IsObject() const;

	bool AsBool(bool bInFallback = false) const;
	int AsInt(int InFallback = 0) const;
	float AsFloat(float InFallback = 0.0f) const;
	double AsDouble(double InFallback = 0.0) const;
	std::string AsString(std::string_view InFallback = {}) const;

	const Array& GetArray() const;
	const Object& GetMembers() const;

	bool Has(std::string_view InKey) const;
	const JsonValue& Get(std::string_view InKey) const;
	const JsonValue& operator[](std::string_view InKey) const;

	bool GetBool(std::string_view InKey, bool bInFallback) const;
	int GetInt(std::string_view InKey, int InFallback) const;
	float GetFloat(std::string_view InKey, float InFallback) const;
	std::string GetString(std::string_view InKey, std::string_view InFallback) const;

	std::string ToString() const;

	bool operator==(const JsonValue& InOther) const;
	bool operator!=(const JsonValue& InOther) const;

private:
	std::variant<std::monostate, bool, int, double, std::string, Array, Object> Data;
};
} // namespace game::model

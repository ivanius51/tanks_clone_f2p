#include "model/JsonValue.h"

#include <array>
#include <charconv>
#include <cstdio>

#include "external/json/document.h"

namespace game::model
{

namespace
{

void WriteString(std::string& OutText, const std::string& InText)
{
	OutText += '"';
	for ( const char Character : InText )
	{
		switch ( Character )
		{
			case '"': OutText += "\\\""; break;
			case '\\': OutText += "\\\\"; break;
			case '\n': OutText += "\\n"; break;
			case '\r': OutText += "\\r"; break;
			case '\t': OutText += "\\t"; break;
			default:
				if ( static_cast<unsigned char>(Character) < 0x20 )
				{
					std::array<char, 8> Escape{};
					std::snprintf(Escape.data(), Escape.size(), "\\u%04x", static_cast<unsigned>(Character));
					OutText += Escape.data();
				}
				else
				{
					OutText += Character;
				}
		}
	}
	OutText += '"';
}

void WriteIndent(std::string& OutText, const int InDepth)
{
	OutText += '\n';
	OutText.append(static_cast<std::size_t>(InDepth), '\t');
}

void WriteValue(std::string& OutText, const JsonValue& InValue, const int InDepth);

void WriteArray(std::string& OutText, const JsonValue::Array& InItems, const int InDepth)
{
	OutText += '[';
	for ( std::size_t Index = 0; Index < InItems.size(); ++Index )
	{
		WriteIndent(OutText, InDepth + 1);
		WriteValue(OutText, InItems[Index], InDepth + 1);
		OutText += Index + 1 < InItems.size() ? "," : "";
	}
	if ( !InItems.empty() )
	{
		WriteIndent(OutText, InDepth);
	}
	OutText += ']';
}

void WriteObject(std::string& OutText, const JsonValue::Object& InMembers, const int InDepth)
{
	OutText += '{';
	std::size_t Index = 0;
	for ( const auto& Member : InMembers )
	{
		WriteIndent(OutText, InDepth + 1);
		WriteString(OutText, Member.first);
		OutText += ": ";
		WriteValue(OutText, Member.second, InDepth + 1);
		OutText += ++Index < InMembers.size() ? "," : "";
	}
	if ( !InMembers.empty() )
	{
		WriteIndent(OutText, InDepth);
	}
	OutText += '}';
}

void WriteValue(std::string& OutText, const JsonValue& InValue, const int InDepth)
{
	switch ( InValue.GetType() )
	{
		case JsonValue::Type::Null: OutText += "null"; break;
		case JsonValue::Type::Bool: OutText += InValue.AsBool() ? "true" : "false"; break;
		case JsonValue::Type::Int: OutText += std::to_string(InValue.AsInt()); break;
		case JsonValue::Type::Double:
		{
			std::array<char, 32> Buffer{};
			const auto Result = std::to_chars(Buffer.data(), Buffer.data() + Buffer.size(), InValue.AsDouble());
			OutText.append(Buffer.data(), Result.ptr);
			break;
		}
		case JsonValue::Type::String: WriteString(OutText, InValue.AsString()); break;
		case JsonValue::Type::Array: WriteArray(OutText, InValue.GetArray(), InDepth); break;
		case JsonValue::Type::Object: WriteObject(OutText, InValue.GetMembers(), InDepth); break;
	}
}

JsonValue FromRapidJson(const rapidjson::Value& InValue)
{
	if ( InValue.IsObject() )
	{
		JsonValue::Object Members;
		for ( auto It = InValue.MemberBegin(); It != InValue.MemberEnd(); ++It )
		{
			Members.emplace(It->name.GetString(), FromRapidJson(It->value));
		}
		return JsonValue(std::move(Members));
	}
	if ( InValue.IsArray() )
	{
		JsonValue::Array Items;
		Items.reserve(InValue.Size());
		for ( rapidjson::SizeType Index = 0; Index < InValue.Size(); ++Index )
		{
			Items.push_back(FromRapidJson(InValue[Index]));
		}
		return JsonValue(std::move(Items));
	}
	if ( InValue.IsBool() )
	{
		return JsonValue(InValue.GetBool());
	}
	if ( InValue.IsString() )
	{
		return JsonValue(std::string(InValue.GetString(), InValue.GetStringLength()));
	}
	if ( InValue.IsInt() )
	{
		return JsonValue(InValue.GetInt());
	}
	if ( InValue.IsNumber() )
	{
		return JsonValue(InValue.GetDouble());
	}
	return JsonValue();
}

const JsonValue& NullValue()
{
	static const JsonValue Null;
	return Null;
}

} // namespace

JsonValue::JsonValue(const bool bInValue) : Data(bInValue) {}
JsonValue::JsonValue(const int InValue) : Data(InValue) {}
JsonValue::JsonValue(const double InValue) : Data(InValue) {}
JsonValue::JsonValue(std::string InValue) : Data(std::move(InValue)) {}
JsonValue::JsonValue(const char* const InValue) : Data(std::string(InValue)) {}
JsonValue::JsonValue(Array InValue) : Data(std::move(InValue)) {}
JsonValue::JsonValue(Object InValue) : Data(std::move(InValue)) {}

JsonValue JsonValue::Parse(const std::string_view InText, bool* const bOutOk)
{
	rapidjson::Document Document;
	Document.Parse<0>(InText.data(), InText.size());
	const bool bOk = !Document.HasParseError();
	if ( bOutOk )
	{
		*bOutOk = bOk;
	}
	return bOk ? FromRapidJson(Document) : JsonValue();
}

JsonValue::Type JsonValue::GetType() const
{
	return static_cast<Type>(Data.index());
}

bool JsonValue::IsNull() const { return GetType() == Type::Null; }
bool JsonValue::IsBool() const { return GetType() == Type::Bool; }
bool JsonValue::IsNumber() const { return GetType() == Type::Int || GetType() == Type::Double; }
bool JsonValue::IsString() const { return GetType() == Type::String; }
bool JsonValue::IsArray() const { return GetType() == Type::Array; }
bool JsonValue::IsObject() const { return GetType() == Type::Object; }

bool JsonValue::AsBool(const bool bInFallback) const
{
	const bool* const Value = std::get_if<bool>(&Data);
	return Value ? *Value : bInFallback;
}

int JsonValue::AsInt(const int InFallback) const
{
	if ( const int* const Value = std::get_if<int>(&Data) )
	{
		return *Value;
	}
	if ( const double* const Value = std::get_if<double>(&Data) )
	{
		return static_cast<int>(*Value);
	}
	return InFallback;
}

float JsonValue::AsFloat(const float InFallback) const
{
	return static_cast<float>(AsDouble(InFallback));
}

double JsonValue::AsDouble(const double InFallback) const
{
	if ( const double* const Value = std::get_if<double>(&Data) )
	{
		return *Value;
	}
	if ( const int* const Value = std::get_if<int>(&Data) )
	{
		return *Value;
	}
	return InFallback;
}

std::string JsonValue::AsString(const std::string_view InFallback) const
{
	const std::string* const Value = std::get_if<std::string>(&Data);
	return Value ? *Value : std::string(InFallback);
}

const JsonValue::Array& JsonValue::GetArray() const
{
	static const Array Empty;
	const Array* const Value = std::get_if<Array>(&Data);
	return Value ? *Value : Empty;
}

const JsonValue::Object& JsonValue::GetMembers() const
{
	static const Object Empty;
	const Object* const Value = std::get_if<Object>(&Data);
	return Value ? *Value : Empty;
}

bool JsonValue::Has(const std::string_view InKey) const
{
	const Object& Members = GetMembers();
	return Members.find(InKey) != Members.end();
}

const JsonValue& JsonValue::Get(const std::string_view InKey) const
{
	const Object& Members = GetMembers();
	const auto FoundIt = Members.find(InKey);
	return FoundIt != Members.end() ? FoundIt->second : NullValue();
}

const JsonValue& JsonValue::operator[](const std::string_view InKey) const
{
	return Get(InKey);
}

bool JsonValue::GetBool(const std::string_view InKey, const bool bInFallback) const
{
	return Get(InKey).AsBool(bInFallback);
}

int JsonValue::GetInt(const std::string_view InKey, const int InFallback) const
{
	return Get(InKey).AsInt(InFallback);
}

float JsonValue::GetFloat(const std::string_view InKey, const float InFallback) const
{
	return Get(InKey).AsFloat(InFallback);
}

std::string JsonValue::GetString(const std::string_view InKey, const std::string_view InFallback) const
{
	return Get(InKey).AsString(InFallback);
}

std::string JsonValue::ToString() const
{
	std::string Text;
	WriteValue(Text, *this, 0);
	Text += '\n';
	return Text;
}

bool JsonValue::operator==(const JsonValue& InOther) const
{
	return Data == InOther.Data;
}

bool JsonValue::operator!=(const JsonValue& InOther) const
{
	return !(*this == InOther);
}

} // namespace game::model

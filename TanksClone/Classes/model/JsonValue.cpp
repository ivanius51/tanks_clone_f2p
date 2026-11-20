#include "model/JsonValue.h"

#include "external/json/document.h"

namespace tanks::model
{

namespace
{

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

bool JsonValue::operator==(const JsonValue& InOther) const
{
	return Data == InOther.Data;
}

bool JsonValue::operator!=(const JsonValue& InOther) const
{
	return !(*this == InOther);
}

} // namespace tanks::model

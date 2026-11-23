#include "model/LevelJsonWriter.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <vector>

namespace tanks::model
{

namespace
{
	constexpr std::array<std::string_view, 10> KnownKeys = {
		"name", "music", "tileset", "fog_of_war", "rows", "enemies", "spawners", "waves", "ai_features", "script"
	};
	// Nested keys come out in this order (reads like the hand-written files); keys not listed follow, sorted.
	constexpr std::array<std::string_view, 29> NestedKeys = {
		"enabled", "vision_tiles", "show_initial_walls", "total", "delay", "max_alive", "interval", "spawn_interval", "types", "enemies",
		"exclude_types", "exclude", "spawns", "spawner", "count", "actors", "steps", "actor", "wait", "move_to", "move_by",
		"line_up", "gap", "face", "shoot", "caption", "until", "seconds", "timeout"
	};
	constexpr size_t MaxInlineLength = 110;
	constexpr std::string_view RowsKey = "rows";
	constexpr double MaxIntegral = 1.0e9;
	constexpr unsigned char FirstPrintable = 0x20;

	std::vector<const JsonValue::Object::value_type*> OrderMembers(const JsonValue::Object& InMembers, const bool bInLevel)
	{
		const auto Contains = [bInLevel](const std::string_view InKey)
		{
			return bInLevel ? LevelJsonWriter::IsKnownKey(InKey) : std::find(NestedKeys.begin(), NestedKeys.end(), InKey) != NestedKeys.end();
		};
		std::vector<const JsonValue::Object::value_type*> Ordered;
		const auto AddInOrder = [&](const auto& InPriority)
		{
			for ( const std::string_view Key : InPriority )
			{
				const auto FoundIt = InMembers.find(Key);
				if ( FoundIt != InMembers.end() )
				{
					Ordered.push_back(&*FoundIt);
				}
			}
		};
		if ( bInLevel )
		{
			AddInOrder(KnownKeys);
		}
		else
		{
			AddInOrder(NestedKeys);
		}
		for ( const auto& Member : InMembers )
		{
			if ( !Contains(Member.first) )
			{
				Ordered.push_back(&Member);
			}
		}
		return Ordered;
	}

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
					if ( static_cast<unsigned char>(Character) < FirstPrintable )
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

	// Always has a '.' so the value reads back as a double, not an int.
	void WriteNumber(std::string& OutText, const double InValue)
	{
		std::array<char, 400> Buffer{};
		const auto Result = std::to_chars(Buffer.data(), Buffer.data() + Buffer.size(), InValue, std::chars_format::fixed);
		const std::string_view Text(Buffer.data(), static_cast<size_t>(Result.ptr - Buffer.data()));
		OutText += Text;
		if ( Text.find('.') == std::string_view::npos )
		{
			OutText += ".0";
		}
	}

	void WriteIndent(std::string& OutText, const int InDepth)
	{
		OutText.append(static_cast<size_t>(InDepth), '\t');
	}

	void WriteValue(std::string& OutText, const JsonValue& InValue, int InDepth, bool bInForceLines);

	void WriteInline(std::string& OutText, const JsonValue& InValue)
	{
		if ( InValue.IsArray() )
		{
			OutText += '[';
			for ( size_t Index = 0; Index < InValue.GetArray().size(); ++Index )
			{
				OutText += Index > 0 ? ", " : "";
				WriteInline(OutText, InValue.GetArray()[Index]);
			}
			OutText += ']';
		}
		else if ( InValue.IsObject() && !InValue.GetMembers().empty() )
		{
			OutText += "{ ";
			const auto Ordered = OrderMembers(InValue.GetMembers(), false);
			for ( size_t Index = 0; Index < Ordered.size(); ++Index )
			{
				OutText += Index > 0 ? ", " : "";
				WriteString(OutText, Ordered[Index]->first);
				OutText += ": ";
				WriteInline(OutText, Ordered[Index]->second);
			}
			OutText += " }";
		}
		else if ( InValue.IsObject() )
		{
			OutText += "{}";
		}
		else
		{
			WriteValue(OutText, InValue, 0, false);
		}
	}

	bool FitsOnLine(const JsonValue& InValue)
	{
		std::string Text;
		WriteInline(Text, InValue);
		return Text.size() <= MaxInlineLength;
	}

	void WriteArray(std::string& OutText, const JsonValue::Array& InItems, const int InDepth, const bool bInInline)
	{
		if ( InItems.empty() )
		{
			OutText += "[]";
			return;
		}
		OutText += bInInline ? "[" : "[\n";
		for ( size_t Index = 0; Index < InItems.size(); ++Index )
		{
			if ( !bInInline )
			{
				WriteIndent(OutText, InDepth + 1);
			}
			WriteValue(OutText, InItems[Index], InDepth + 1, false);
			OutText += Index + 1 < InItems.size() ? (bInInline ? ", " : ",\n") : "";
		}
		if ( !bInInline )
		{
			OutText += '\n';
			WriteIndent(OutText, InDepth);
		}
		OutText += ']';
	}

	void WriteObject(std::string& OutText, const JsonValue::Object& InMembers, const int InDepth, const bool bInInline, const bool bInLevel)
	{
		if ( InMembers.empty() )
		{
			OutText += "{}";
			return;
		}
		const auto Ordered = OrderMembers(InMembers, bInLevel);

		OutText += bInInline ? "{ " : "{\n";
		for ( size_t Index = 0; Index < Ordered.size(); ++Index )
		{
			if ( !bInInline )
			{
				WriteIndent(OutText, InDepth + 1);
			}
			WriteString(OutText, Ordered[Index]->first);
			OutText += ": ";
			WriteValue(OutText, Ordered[Index]->second, InDepth + 1, bInLevel && Ordered[Index]->first == RowsKey);
			OutText += Index + 1 < Ordered.size() ? (bInInline ? ", " : ",\n") : "";
		}
		if ( bInInline )
		{
			OutText += " }";
			return;
		}
		OutText += '\n';
		WriteIndent(OutText, InDepth);
		OutText += '}';
	}

	void WriteValue(std::string& OutText, const JsonValue& InValue, const int InDepth, const bool bInForceLines)
	{
		switch ( InValue.GetType() )
		{
			case JsonValue::Type::Null: OutText += "null"; break;
			case JsonValue::Type::Bool: OutText += InValue.AsBool() ? "true" : "false"; break;
			case JsonValue::Type::Int: OutText += std::to_string(InValue.AsInt()); break;
			case JsonValue::Type::Double: WriteNumber(OutText, InValue.AsDouble()); break;
			case JsonValue::Type::String: WriteString(OutText, InValue.AsString()); break;
			case JsonValue::Type::Array: WriteArray(OutText, InValue.GetArray(), InDepth, !bInForceLines && FitsOnLine(InValue)); break;
			case JsonValue::Type::Object: WriteObject(OutText, InValue.GetMembers(), InDepth, FitsOnLine(InValue), false); break;
		}
	}
}

const std::array<std::string_view, 10>& LevelJsonWriter::GetKnownKeys()
{
	return KnownKeys;
}

bool LevelJsonWriter::IsKnownKey(const std::string_view InKey)
{
	return std::find(KnownKeys.begin(), KnownKeys.end(), InKey) != KnownKeys.end();
}

std::string LevelJsonWriter::Write(const JsonValue& InRoot)
{
	std::string Text;
	if ( InRoot.IsObject() )
	{
		WriteObject(Text, InRoot.GetMembers(), 0, false, true);
	}
	else
	{
		WriteValue(Text, InRoot, 0, false);
	}
	Text += '\n';
	return Text;
}

JsonValue LevelJsonWriter::FromFloat(const float InValue)
{
	std::array<char, 40> Buffer{};
	const auto Written = std::to_chars(Buffer.data(), Buffer.data() + Buffer.size(), InValue);
	double Result = InValue;
	std::from_chars(Buffer.data(), Written.ptr, Result);
	return JsonValue(Result);
}

JsonValue LevelJsonWriter::FromNode(const IConfigNode& InNode)
{
	if ( const std::optional<bool> Flag = InNode.ReadBool() )
	{
		return JsonValue(*Flag);
	}
	if ( const std::optional<double> Number = InNode.ReadNumber() )
	{
		const bool bIntegral = std::floor(*Number) == *Number && std::fabs(*Number) < MaxIntegral;
		return bIntegral ? JsonValue(static_cast<int>(*Number)) : JsonValue(*Number);
	}
	if ( const std::optional<std::string> Text = InNode.ReadString() )
	{
		return JsonValue(*Text);
	}
	if ( InNode.GetItemCount() > 0 )
	{
		JsonValue::Array Items;
		for ( size_t Index = 0; Index < InNode.GetItemCount(); ++Index )
		{
			Items.push_back(FromNode(*InNode.GetItem(Index)));
		}
		return JsonValue(std::move(Items));
	}
	JsonValue::Object Members;
	for ( const std::string& Key : InNode.GetKeys() )
	{
		Members.emplace(Key, FromNode(*InNode.FindChild(Key)));
	}
	return JsonValue(std::move(Members));
}

} // namespace tanks::model

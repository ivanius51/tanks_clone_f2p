#include "core/JsonConfigNode.h"

#include "cocos2d.h"

namespace game
{

JsonConfigNode::JsonConfigNode(const model::JsonValue& InValue, std::string_view InSource)
	: Source(InSource)
{
	if ( InValue.IsObject() )
	{
		for ( const auto& [Key, Child] : InValue.GetMembers() )
		{
			Members.emplace_back(Key, MakeUnique<JsonConfigNode>(Child, Source + " / " + Key));
		}
	}
	else if ( InValue.IsArray() )
	{
		const model::JsonValue::Array& Elements = InValue.GetArray();
		for ( std::size_t Index = 0; Index < Elements.size(); ++Index )
		{
			Items.push_back(MakeUnique<JsonConfigNode>(Elements[Index], Source + " [" + std::to_string(Index) + "]"));
		}
	}
	else
	{
		Value = InValue;
	}
}

THandle<JsonConfigNode> JsonConfigNode::Parse(std::string_view InText, std::string_view InSource, bool* const bOutOk)
{
	bool bOk = false;
	const model::JsonValue Root = model::JsonValue::Parse(InText, &bOk);
	if ( !bOk )
	{
		CCLOGERROR("Config: failed to parse %s", std::string(InSource).c_str());
	}
	if ( bOutOk )
	{
		*bOutOk = bOk;
	}
	return MakeHandle<JsonConfigNode>(Root, InSource);
}

const model::IConfigNode* JsonConfigNode::FindChild(std::string_view InKey) const
{
	for ( const auto& [Key, Child] : Members )
	{
		if ( Key == InKey )
		{
			return Child.Get();
		}
	}
	return nullptr;
}

std::vector<std::string> JsonConfigNode::GetKeys() const
{
	std::vector<std::string> Keys;
	Keys.reserve(Members.size());
	for ( const auto& Member : Members )
	{
		Keys.push_back(Member.first);
	}
	return Keys;
}

std::size_t JsonConfigNode::GetItemCount() const
{
	return Items.size();
}

const model::IConfigNode* JsonConfigNode::GetItem(const std::size_t InIndex) const
{
	return InIndex < Items.size() ? Items[InIndex].Get() : nullptr;
}

std::optional<bool> JsonConfigNode::ReadBool() const
{
	return Value.IsBool() ? std::optional<bool>(Value.AsBool()) : std::nullopt;
}

std::optional<double> JsonConfigNode::ReadNumber() const
{
	return Value.IsNumber() ? std::optional<double>(Value.AsDouble()) : std::nullopt;
}

std::optional<std::string> JsonConfigNode::ReadString() const
{
	return Value.IsString() ? std::optional<std::string>(Value.AsString()) : std::nullopt;
}

void JsonConfigNode::ReportMissing([[maybe_unused]] std::string_view InKey) const
{
	CCLOGERROR("Config: \"%s\" is missing or has a wrong type in %s", std::string(InKey).c_str(), Source.c_str());
}

} // namespace game

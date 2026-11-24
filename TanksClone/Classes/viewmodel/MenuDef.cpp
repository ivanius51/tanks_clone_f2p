#include "viewmodel/MenuDef.h"

#include <utility>

namespace tanks::viewmodel
{

MenuItemDef MenuItemDef::FromConfig(const model::IConfigNode& InNode)
{
	MenuItemDef Result;
	Result.Id = InNode.GetValueOr("id", "");
	Result.Kind = InNode.GetValueOr("kind", "");
	Result.OptionsSource = InNode.GetValueOr("options_source", "");
	Result.Min = InNode.GetValueOr("min", 0.0f);
	Result.Max = InNode.GetValueOr("max", 0.0f);
	Result.Step = InNode.GetValueOr("step", 0.0f);
	const model::IConfigNode* const OptionsNode = InNode.FindChild("options");
	for ( std::size_t Index = 0; OptionsNode && Index < OptionsNode->GetItemCount(); ++Index )
	{
		Result.Options.push_back(OptionsNode->GetItem(Index)->ReadString().value_or(""));
	}
	Result.TextKey = InNode.GetValueOr("text", "");
	Result.ValueId = InNode.GetValueOr("value", "");
	Result.Sample = InNode.GetValueOr("sample", "");
	Result.Action = InNode.GetValueOr("action", "");
	Result.When = InNode.GetValueOr("when", "");
	Result.EnabledWhen = InNode.GetValueOr("enabled_when", "");
	Result.Slot = InNode.GetValueOr("slot", "");
	Result.bDebug = InNode.GetValueOr("debug", false);
	return Result;
}

model::JsonValue MenuItemDef::ToConfig() const
{
	model::JsonValue::Object Out;
	const auto PutText = [&Out](const char* const InKey, const std::string& InValue)
	{
		if ( !InValue.empty() )
		{
			Out[InKey] = model::JsonValue(InValue);
		}
	};
	PutText("id", Id);
	PutText("kind", Kind);
	PutText("options_source", OptionsSource);
	if ( !Options.empty() )
	{
		model::JsonValue::Array OptionsOut;
		for ( const std::string& Current : Options )
		{
			OptionsOut.push_back(model::JsonValue(Current));
		}
		Out["options"] = model::JsonValue(std::move(OptionsOut));
	}
	if ( Kind == "stepper" )
	{
		Out["min"] = model::JsonValue(static_cast<int>(Min));
		Out["max"] = model::JsonValue(static_cast<int>(Max));
		Out["step"] = model::JsonValue(static_cast<int>(Step));
	}
	PutText("text", TextKey);
	PutText("value", ValueId);
	PutText("sample", Sample);
	PutText("action", Action);
	PutText("when", When);
	PutText("enabled_when", EnabledWhen);
	PutText("slot", Slot);
	if ( bDebug )
	{
		Out["debug"] = model::JsonValue(true);
	}
	return model::JsonValue(std::move(Out));
}

model::JsonValue MenuDef::ToConfig() const
{
	model::JsonValue::Object Out;
	Out["root"] = model::JsonValue(Root);
	Out["container"] = model::JsonValue(Container);
	Out["button_style"] = model::JsonValue(ButtonStyle);
	model::JsonValue::Object TextsOut;
	for ( const MenuTextDef& Current : Texts )
	{
		model::JsonValue::Object Entry;
		if ( !Current.Key.empty() )
		{
			Entry["key"] = model::JsonValue(Current.Key);
		}
		if ( !Current.Sample.empty() )
		{
			Entry["sample"] = model::JsonValue(Current.Sample);
		}
		TextsOut[Current.Child] = model::JsonValue(std::move(Entry));
	}
	Out["texts"] = model::JsonValue(std::move(TextsOut));
	model::JsonValue::Array ItemsOut;
	for ( const MenuItemDef& Current : Items )
	{
		ItemsOut.push_back(Current.ToConfig());
	}
	Out["items"] = model::JsonValue(std::move(ItemsOut));
	return model::JsonValue(std::move(Out));
}

MenuDef MenuDef::FromConfig(const model::IConfigNode& InNode, const std::string& InId)
{
	MenuDef Result;
	Result.Id = InId;
	Result.Root = InNode.GetString("root");
	Result.Container = InNode.GetValueOr("container", "buttons");
	Result.ButtonStyle = InNode.GetValueOr("button_style", "overlay");
	const model::IConfigNode* const TextsNode = InNode.FindChild("texts");
	for ( const std::string& Child : TextsNode ? TextsNode->GetKeys() : std::vector<std::string>() )
	{
		const model::IConfigNode* const Entry = TextsNode->FindChild(Child);
		Result.Texts.push_back({ Child, Entry->GetValueOr("key", ""), Entry->GetValueOr("sample", "") });
	}
	const model::IConfigNode* const ItemsNode = InNode.FindChild("items");
	for ( std::size_t Index = 0; ItemsNode && Index < ItemsNode->GetItemCount(); ++Index )
	{
		Result.Items.push_back(MenuItemDef::FromConfig(*ItemsNode->GetItem(Index)));
	}
	return Result;
}

const MenuTextDef* MenuDef::FindTextDef(const std::string& InChild) const
{
	for ( const MenuTextDef& Current : Texts )
	{
		if ( Current.Child == InChild )
		{
			return &Current;
		}
	}
	return nullptr;
}

} // namespace tanks::viewmodel

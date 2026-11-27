#include "viewmodel/MenuDef.h"
#include "templates/TContainers.h"

#include <utility>

namespace game::viewmodel
{

MenuItemDef MenuItemDef::FromConfig(const model::IConfigNode& InNode)
{
	MenuItemDef Result;
	Result.Id = InNode.GetValueOr("id", "");
	Result.Kind = InNode.GetValueOr("kind", "");
	Result.CheckedWhen = InNode.GetValueOr("checked_when", "");
	Result.OptionsSource = InNode.GetValueOr("options_source", "");
	Result.OffText = InNode.GetValueOr("off_text", "");
	Result.Suffix = InNode.GetValueOr("suffix", "");
	Result.Min = InNode.GetValueOr("min", 0.0f);
	Result.Max = InNode.GetValueOr("max", 0.0f);
	Result.Step = InNode.GetValueOr("step", 0.0f);
	const model::IConfigNode* const OptionsNode = InNode.FindChild("options");
	for ( std::size_t Index = 0; OptionsNode && Index < OptionsNode->GetItemCount(); ++Index )
	{
		Result.Options.PushBack(OptionsNode->GetItem(Index)->ReadString().value_or(""));
	}
	Result.TextKey = InNode.GetValueOr("text", "");
	Result.ValueId = InNode.GetValueOr("value", "");
	Result.Sample = InNode.GetValueOr("sample", "");
	Result.Action = InNode.GetValueOr("action", "");
	Result.When = InNode.GetValueOr("when", "");
	Result.EnabledWhen = InNode.GetValueOr("enabled_when", "");
	Result.Slot = InNode.GetValueOr("slot", "");
	Result.Style = InNode.GetValueOr("style", "");
	Result.SubtitleId = InNode.GetValueOr("subtitle", "");
	Result.Container = InNode.GetValueOr("container", "");
	Result.Width = InNode.GetValueOr("width", 0.0f);
	Result.Height = InNode.GetValueOr("height", 0.0f);
	Result.FontSize = InNode.GetValueOr("font_size", 0.0f);
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
	PutText("checked_when", CheckedWhen);
	PutText("options_source", OptionsSource);
	PutText("off_text", OffText);
	PutText("suffix", Suffix);
	if ( !Options.IsEmpty() )
	{
		model::JsonValue::Array OptionsOut;
		for ( const std::string& Current : Options )
		{
			OptionsOut.PushBack(model::JsonValue(Current));
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
	PutText("style", Style);
	PutText("subtitle", SubtitleId);
	PutText("container", Container);
	if ( Width != 0.0f )
	{
		Out["width"] = model::JsonValue(static_cast<int>(Width));
	}
	if ( Height != 0.0f )
	{
		Out["height"] = model::JsonValue(static_cast<int>(Height));
	}
	if ( FontSize != 0.0f )
	{
		Out["font_size"] = model::JsonValue(static_cast<int>(FontSize));
	}
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
	if ( Button.Width != 0.0f || Button.Height != 0.0f || Button.Spacing != 0.0f )
	{
		model::JsonValue::Object ButtonOut;
		const auto Put = [&ButtonOut](const char* const InKey, const float InValue)
		{
			if ( InValue != 0.0f )
			{
				ButtonOut[InKey] = model::JsonValue(static_cast<int>(InValue));
			}
		};
		Put("width", Button.Width);
		Put("height", Button.Height);
		Put("spacing", Button.Spacing);
		Out["button"] = model::JsonValue(std::move(ButtonOut));
	}
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
		ItemsOut.PushBack(Current.ToConfig());
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
	if ( const model::IConfigNode* const ButtonNode = InNode.FindChild("button") )
	{
		Result.Button.Width = ButtonNode->GetValueOr("width", 0.0f);
		Result.Button.Height = ButtonNode->GetValueOr("height", 0.0f);
		Result.Button.Spacing = ButtonNode->GetValueOr("spacing", 0.0f);
	}
	const model::IConfigNode* const TextsNode = InNode.FindChild("texts");
	for ( const std::string& Child : TextsNode ? TextsNode->GetKeys() : TArray<std::string>() )
	{
		const model::IConfigNode* const Entry = TextsNode->FindChild(Child);
		Result.Texts.PushBack({ Child, Entry->GetValueOr("key", ""), Entry->GetValueOr("sample", "") });
	}
	const model::IConfigNode* const ItemsNode = InNode.FindChild("items");
	for ( std::size_t Index = 0; ItemsNode && Index < ItemsNode->GetItemCount(); ++Index )
	{
		Result.Items.PushBack(MenuItemDef::FromConfig(*ItemsNode->GetItem(Index)));
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

} // namespace game::viewmodel

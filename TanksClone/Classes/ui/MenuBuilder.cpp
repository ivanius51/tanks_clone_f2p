#include "ui/MenuBuilder.h"

#include "cocos2d.h"

#include "core/BuildInfo.h"
#include "core/GameContext.h"
#include "services/StringsManager.h"

namespace tanks::view::cocos
{

namespace
{
	template <typename TFunction>
	const TFunction* Find(const std::map<std::string, TFunction>& InMap, const std::string& InId, const char* const InKind)
	{
		const auto Found = InMap.find(InId);
		if ( Found == InMap.end() )
		{
			cocos2d::log("MenuBuilder: no %s \"%s\"", InKind, InId.c_str());
			return nullptr;
		}
		return &Found->second;
	}

	bool Holds(const std::string& InCondition, const MenuBindings& InBindings)
	{
		if ( InCondition.empty() )
		{
			return true;
		}
		const std::function<bool()>* const Condition = Find(InBindings.Conditions, InCondition, "condition");
		return Condition && (*Condition)();
	}
}

std::string MenuText(const viewmodel::MenuTextDef& InText)
{
	return InText.Key.empty() ? InText.Sample : GameContext::Get().GetManager<StringsManager>().GetString(InText.Key);
}

namespace
{
	std::string Decorate(const viewmodel::MenuItemDef& InItem, const std::string& InCaption, const bool bInChecked)
	{
		if ( InItem.Kind == "choice" )
		{
			return "<  " + InCaption + "  >";
		}
		if ( InItem.Kind == "stepper" )
		{
			return "-  " + InCaption + "  +";
		}
		if ( InItem.Kind == "toggle" )
		{
			return (bInChecked ? "[X]  " : "[ ]  ") + InCaption;
		}
		return InCaption;
	}
}

std::string MenuCaption(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, const bool bInEditor)
{
	std::string Caption = GameContext::Get().GetManager<StringsManager>().GetString(InItem.TextKey);
	if ( !InItem.ValueId.empty() )
	{
		if ( bInEditor )
		{
			Caption += ": " + InItem.Sample;
		}
		else
		{
			const std::function<std::string()>* const Value = Find(InBindings.Values, InItem.ValueId, "value");
			Caption += ": " + (Value ? (*Value)() : std::string());
		}
	}
	return Decorate(InItem, Caption, bInEditor || Holds(InItem.CheckedWhen, InBindings));
}

std::vector<MenuEntry> ResolveMenuEntries(const viewmodel::MenuDef& InMenu, const MenuBindings& InBindings, const bool bInEditor)
{
	std::vector<MenuEntry> Result;
	for ( const viewmodel::MenuItemDef& Item : InMenu.Items )
	{
		if ( !bInEditor && ((Item.bDebug && !bHasDebugTools) || !Holds(Item.When, InBindings)) )
		{
			continue;
		}
		if ( !Item.Slot.empty() )
		{
			const auto Slot = InBindings.Slots.find(Item.Slot);
			if ( bInEditor && Slot == InBindings.Slots.end() )
			{
				Result.push_back({ &Item, "[" + Item.Slot + "]", {}, {}, false });
			}
			if ( Slot != InBindings.Slots.end() )
			{
				for ( const auto& Extra : Slot->second )
				{
					Result.push_back({ &Item, Extra.first, bInEditor ? std::function<void()>() : Extra.second, {}, true });
				}
			}
			continue;
		}
		MenuEntry Entry;
		Entry.Item = &Item;
		Entry.Caption = MenuCaption(Item, InBindings, bInEditor);
		if ( !bInEditor )
		{
			const std::function<void()>* const Action = Item.Action.empty() ? nullptr : Find(InBindings.Actions, Item.Action, "action");
			Entry.Action = Action ? *Action : std::function<void()>();
			const auto Adjust = InBindings.Adjusts.find(Item.Action);
			Entry.Adjust = Adjust == InBindings.Adjusts.end() ? std::function<void(int)>() : Adjust->second;
			Entry.bEnabled = Holds(Item.EnabledWhen, InBindings);
		}
		Result.push_back(std::move(Entry));
	}
	return Result;
}

} // namespace tanks::view::cocos

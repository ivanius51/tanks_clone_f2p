#include "ui/MenuBuilder.h"

#include <algorithm>
#include <charconv>

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

bool MenuChecked(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, const bool bInEditor)
{
	return bInEditor || Holds(InItem.CheckedWhen, InBindings);
}

namespace
{
	bool ParseNumber(const std::string& InText, int& OutNumber)
	{
		const char* const End = InText.data() + InText.size();
		const auto Parsed = std::from_chars(InText.data(), End, OutNumber);
		return !InText.empty() && Parsed.ec == std::errc() && Parsed.ptr == End;
	}

	std::string OptionCaption(const viewmodel::MenuItemDef& InItem, const std::string& InOption)
	{
		const StringsManager& Strings = GameContext::Get().GetManager<StringsManager>();
		int Number = 0;
		if ( ParseNumber(InOption, Number) )
		{
			return Number <= 0 && !InItem.OffText.empty() ? Strings.GetString(InItem.OffText) : InOption + InItem.Suffix;
		}
		const LanguageDef* const Language = Strings.FindLanguage(InOption);
		return Language ? Language->Name : Strings.GetString(InOption);
	}

	void BindChoice(const viewmodel::MenuItemDef& InItem, const MenuChoiceBinding& InBinding, MenuEntry& OutEntry)
	{
		const std::vector<std::string> Options = InItem.Options;
		OutEntry.Adjust = [InBinding, Options](const int InDirection)
		{
			const int Count = static_cast<int>(Options.size());
			const int Current = static_cast<int>(std::find(Options.begin(), Options.end(), InBinding.Get()) - Options.begin());
			InBinding.Set(Options[static_cast<size_t>(((Current + InDirection) % Count + Count) % Count)]);
		};
		OutEntry.Action = [Adjust = OutEntry.Adjust]() { Adjust(1); };
	}
}

std::string MenuCaption(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, const bool bInEditor)
{
	std::string Caption = GameContext::Get().GetManager<StringsManager>().GetString(InItem.TextKey);
	const auto Index = InBindings.Indexes.find(InItem.Id);
	const auto Choice = InBindings.Choices.find(InItem.Id);
	if ( !bInEditor && Index != InBindings.Indexes.end() && !InItem.Options.empty() )
	{
		const int Current = std::clamp(Index->second.Get(), 0, static_cast<int>(InItem.Options.size()) - 1);
		Caption += ": " + OptionCaption(InItem, InItem.Options[static_cast<size_t>(Current)]);
	}
	else if ( !bInEditor && Choice != InBindings.Choices.end() && !InItem.Options.empty() )
	{
		Caption += ": " + OptionCaption(InItem, Choice->second.Get());
	}
	else if ( !InItem.ValueId.empty() )
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
	return Caption;
}

namespace
{
	void BindFromData(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, MenuEntry& OutEntry)
	{
		const auto Index = InBindings.Indexes.find(InItem.Id);
		const auto Number = InBindings.Numbers.find(InItem.Id);
		const auto Choice = InBindings.Choices.find(InItem.Id);
		if ( InItem.Kind == "choice" && !InItem.Options.empty() && Choice != InBindings.Choices.end() )
		{
			BindChoice(InItem, Choice->second, OutEntry);
		}
		else if ( InItem.Kind == "choice" && !InItem.Options.empty() && Index != InBindings.Indexes.end() )
		{
			const MenuNumberBinding Binding = Index->second;
			const int Count = static_cast<int>(InItem.Options.size());
			OutEntry.Adjust = [Binding, Count](const int InDirection) { Binding.Set(((Binding.Get() + InDirection) % Count + Count) % Count); };
			OutEntry.Action = [Adjust = OutEntry.Adjust]() { Adjust(1); };
		}
		else if ( InItem.Kind == "stepper" && Number != InBindings.Numbers.end() )
		{
			const MenuNumberBinding Binding = Number->second;
			const int Min = static_cast<int>(InItem.Min);
			const int Max = static_cast<int>(InItem.Max);
			const int Step = std::max(1, static_cast<int>(InItem.Step));
			OutEntry.Adjust = [Binding, Min, Max, Step](const int InDirection) { Binding.Set(std::clamp(Binding.Get() + InDirection * Step, Min, Max)); };
			OutEntry.Action = [Binding, Min, Max, Step]() { Binding.Set(Binding.Get() <= Min ? Max : std::max(Binding.Get() - Step, Min)); };
		}
	}
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
		Entry.bChecked = MenuChecked(Item, InBindings, bInEditor);
		if ( !bInEditor )
		{
			const std::function<void()>* const Action = Item.Action.empty() ? nullptr : Find(InBindings.Actions, Item.Action, "action");
			Entry.Action = Action ? *Action : std::function<void()>();
			const auto Adjust = InBindings.Adjusts.find(Item.Action);
			Entry.Adjust = Adjust == InBindings.Adjusts.end() ? std::function<void(int)>() : Adjust->second;
			BindFromData(Item, InBindings, Entry);
			Entry.bEnabled = Holds(Item.EnabledWhen, InBindings);
		}
		Result.push_back(std::move(Entry));
	}
	return Result;
}

} // namespace tanks::view::cocos

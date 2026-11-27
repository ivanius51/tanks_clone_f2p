#include "templates/TContainers.h"
#include "model/Log.h"
#include "ui/MenuBuilder.h"

#include <algorithm>
#include <charconv>
#include <memory>
#include <utility>
#include "templates/TFunction.h"

#include "cocos2d.h"

#include "core/BuildInfo.h"
#include "core/GameContext.h"
#include "services/StringsManager.h"

namespace game::view::cocos
{

namespace
{
	template <typename TFunction>
	const TFunction* Find(const TMap<FPersistentString, TFunction>& InMap, const std::string& InId, const char* const InKind)
	{
		const auto* const Found = InMap.Find(InId);
		if ( !Found )
		{
			model::Log::Info("MenuBuilder: no %s \"%s\"", InKind, InId.c_str());
			return nullptr;
		}
		return Found;
	}

	bool Holds(const std::string& InCondition, const MenuBindings& InBindings)
	{
		if ( InCondition.empty() )
		{
			return true;
		}
		const TFunction<bool()>* const Condition = Find(InBindings.Conditions, InCondition, "condition");
		return Condition && (*Condition)();
	}
}

std::string MenuText(const viewmodel::MenuTextDef& InText)
{
	return InText.Key.empty() ? InText.Sample : GameContext::Get<StringsManager>().GetString(InText.Key);
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
		const StringsManager& Strings = GameContext::Get<StringsManager>();
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
		const auto Shared = MakeSharedValue<const std::pair<MenuChoiceBinding, TArray<std::string>>>(InBinding, InItem.Options);
		OutEntry.Adjust = [Shared](const int InDirection)
		{
			const TArray<std::string>& Options = Shared->Get().second;
			const MenuChoiceBinding& Binding = Shared->Get().first;
			const int Count = static_cast<int>(Options.Size());
			const int Current = static_cast<int>(std::find(Options.begin(), Options.end(), Binding.Get()) - Options.begin());
			Binding.Set(Options[static_cast<size_t>(((Current + InDirection) % Count + Count) % Count)]);
		};
		const auto Adjust = MakeSharedValue<const TFunction<void(int), UiCallbackInlineSize>>(OutEntry.Adjust);
		OutEntry.Action = [Adjust]() { Adjust->Get()(1); };
	}
}

std::string MenuCaption(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, const bool bInEditor)
{
	std::string Caption = GameContext::Get<StringsManager>().GetString(InItem.TextKey);
	auto* const Index = InBindings.Indexes.Find(InItem.Id);
	auto* const Choice = InBindings.Choices.Find(InItem.Id);
	if ( !bInEditor && Index && !InItem.Options.IsEmpty() )
	{
		const int Current = std::clamp(Index->Get(), 0, static_cast<int>(InItem.Options.Size()) - 1);
		Caption += ": " + OptionCaption(InItem, InItem.Options[static_cast<size_t>(Current)]);
	}
	else if ( !bInEditor && Choice && !InItem.Options.IsEmpty() )
	{
		Caption += ": " + OptionCaption(InItem, Choice->Get());
	}
	else if ( !InItem.ValueId.empty() )
	{
		if ( bInEditor )
		{
			Caption += ": " + InItem.Sample;
		}
		else
		{
			const TFunction<std::string()>* const Value = Find(InBindings.Values, InItem.ValueId, "value");
			Caption += ": " + (Value ? (*Value)() : std::string());
		}
	}
	return Caption;
}

namespace
{
	void BindFromData(const viewmodel::MenuItemDef& InItem, const MenuBindings& InBindings, MenuEntry& OutEntry)
	{
		auto* const Index = InBindings.Indexes.Find(InItem.Id);
		auto* const Number = InBindings.Numbers.Find(InItem.Id);
		auto* const Choice = InBindings.Choices.Find(InItem.Id);
		if ( InItem.Kind == "choice" && !InItem.Options.IsEmpty() && Choice )
		{
			BindChoice(InItem, *Choice, OutEntry);
		}
		else if ( InItem.Kind == "choice" && !InItem.Options.IsEmpty() && Index )
		{
			const auto Shared = MakeSharedValue<const MenuNumberBinding>(*Index);
			const int Count = static_cast<int>(InItem.Options.Size());
			OutEntry.Adjust = [Shared, Count](const int InDirection) { Shared->Get().Set(((Shared->Get().Get() + InDirection) % Count + Count) % Count); };
			const auto Adjust = MakeSharedValue<const TFunction<void(int), UiCallbackInlineSize>>(OutEntry.Adjust);
			OutEntry.Action = [Adjust]() { Adjust->Get()(1); };
		}
		else if ( (InItem.Kind == "stepper" || InItem.Kind == "slider") && Number )
		{
			const auto Shared = MakeSharedValue<const MenuNumberBinding>(*Number);
			const int Min = static_cast<int>(InItem.Min);
			const int Max = static_cast<int>(InItem.Max);
			const int Step = std::max(1, static_cast<int>(InItem.Step));
			OutEntry.Adjust = [Shared, Min, Max, Step](const int InDirection) { Shared->Get().Set(std::clamp(Shared->Get().Get() + InDirection * Step, Min, Max)); };
			OutEntry.Action = [Shared, Min, Max, Step]() { Shared->Get().Set(Shared->Get().Get() <= Min ? Max : std::max(Shared->Get().Get() - Step, Min)); };
			OutEntry.Slide = [Shared, Min, Max, Step](const float InFraction)
			{
				const int Steps = static_cast<int>(std::lround(InFraction * static_cast<float>(Max - Min) / static_cast<float>(Step)));
				const int Value = std::clamp(Min + Steps * Step, Min, Max);
				if ( Value != Shared->Get().Get() )
				{
					Shared->Get().Set(Value);
				}
			};
		}
	}
}

TArray<MenuEntry> ResolveMenuEntries(const viewmodel::MenuDef& InMenu, const MenuBindings& InBindings, const bool bInEditor)
{
	TArray<MenuEntry> Result;
	for ( const viewmodel::MenuItemDef& Item : InMenu.Items )
	{
		if ( !bInEditor && ((Item.bDebug && !bHasDebugTools) || !Holds(Item.When, InBindings)) )
		{
			continue;
		}
		if ( !Item.Slot.empty() )
		{
			auto* const Slot = InBindings.Slots.Find(Item.Slot);
			if ( bInEditor && !Slot )
			{
				Result.PushBack({ &Item, "[" + Item.Slot + "]", {}, {}, false });
			}
			if ( Slot )
			{
				for ( const auto& Extra : *Slot )
				{
					Result.PushBack({ &Item, Extra.first, bInEditor ? TFunction<void(), UiCallbackInlineSize>() : Extra.second, {}, true });
				}
			}
			continue;
		}
		MenuEntry Entry;
		Entry.Item = &Item;
		Entry.Caption = MenuCaption(Item, InBindings, bInEditor);
		Entry.bChecked = MenuChecked(Item, InBindings, bInEditor);
		if ( !Item.SubtitleId.empty() )
		{
			auto* const Value = InBindings.Values.Find(Item.SubtitleId);
			Entry.Subtitle = Value ? (*Value)() : "[" + Item.SubtitleId + "]";
		}
		if ( !bInEditor )
		{
			const TFunction<void(), UiCallbackInlineSize>* const Action = Item.Action.empty() ? nullptr : Find(InBindings.Actions, Item.Action, "action");
			Entry.Action = Action ? *Action : TFunction<void(), UiCallbackInlineSize>();
			auto* const Adjust = InBindings.Adjusts.Find(Item.Action);
			Entry.Adjust = Adjust ? *Adjust : TFunction<void(int), UiCallbackInlineSize>();
			BindFromData(Item, InBindings, Entry);
			Entry.bEnabled = Holds(Item.EnabledWhen, InBindings);
		}
		Result.PushBack(std::move(Entry));
	}
	return Result;
}

} // namespace game::view::cocos

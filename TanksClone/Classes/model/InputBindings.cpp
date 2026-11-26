#include "model/InputBindings.h"
#include "templates/TContainers.h"

#include <algorithm>

namespace game::model
{

namespace
{
	static_assert(static_cast<int>(InputAction::SelectItem9) - static_cast<int>(InputAction::SelectItem1) == 8, "the item actions are in order");

	constexpr const char* ContextsKey = "contexts";
	constexpr const char* PadLabelsKey = "pad_labels";
	constexpr const char* DebugKey = "debug";

	std::size_t IndexOf(const InputContext InContext)
	{
		return static_cast<std::size_t>(InContext);
	}
}

std::string GetContextName(const InputContext InContext)
{
	switch ( InContext )
	{
		case InputContext::Menu: return "menu";
		case InputContext::Gameplay: return "gameplay";
		case InputContext::Editor: return "editor";
		case InputContext::Debug: return "debug";
		default: return "";
	}
}

InputContext FindContext(std::string_view InName)
{
	for ( std::size_t Index = 0; Index < InputContextCount; ++Index )
	{
		if ( GetContextName(static_cast<InputContext>(Index)) == InName )
		{
			return static_cast<InputContext>(Index);
		}
	}
	return InputContext::Count;
}

std::string GetActionName(const InputAction InAction)
{
	if ( InAction >= InputAction::SelectItem1 && InAction <= InputAction::SelectItem9 )
	{
		return "select_item_" + std::to_string(static_cast<int>(InAction) - static_cast<int>(InputAction::SelectItem1) + 1);
	}
	switch ( InAction )
	{
		case InputAction::UiUp: return "ui_up";
		case InputAction::UiDown: return "ui_down";
		case InputAction::UiConfirm: return "ui_confirm";
		case InputAction::UiBack: return "ui_back";
		case InputAction::UiLeft: return "ui_left";
		case InputAction::UiRight: return "ui_right";
		case InputAction::MoveUp: return "move_up";
		case InputAction::MoveDown: return "move_down";
		case InputAction::MoveLeft: return "move_left";
		case InputAction::MoveRight: return "move_right";
		case InputAction::Fire: return "fire";
		case InputAction::Ability1: return "ability_1";
		case InputAction::SwitchWeapon: return "switch_weapon";
		case InputAction::Pause: return "pause";
		case InputAction::NextTab: return "next_tab";
		case InputAction::Back: return "back";
		case InputAction::ToggleStats: return "toggle_stats";
		case InputAction::ToggleAiDebug: return "toggle_ai_debug";
		default: return "";
	}
}

InputAction FindAction(std::string_view InName)
{
	for ( std::size_t Index = 0; Index < InputActionCount; ++Index )
	{
		if ( GetActionName(static_cast<InputAction>(Index)) == InName )
		{
			return static_cast<InputAction>(Index);
		}
	}
	return InputAction::Count;
}

InputBindings InputBindings::FromConfig(const IConfigNode& InNode, const bool bInWithDebug)
{
	InputBindings Result;
	if ( const IConfigNode* Contexts = InNode.FindChild(ContextsKey) )
	{
		for ( const std::string& Name : Contexts->GetKeys() )
		{
			const InputContext Context = FindContext(Name);
			if ( Context == InputContext::Count || Context == InputContext::Debug )
			{
				Result.Errors.PushBack("unknown input context '" + Name + "'");
				continue;
			}
			Result.ReadContext(Context, *Contexts->FindChild(Name));
		}
	}
	else
	{
		Result.Errors.PushBack("no \"contexts\" section");
	}

	const IConfigNode* Debug = InNode.FindChild(DebugKey);
	if ( bInWithDebug && Debug )
	{
		Result.ReadContext(InputContext::Debug, *Debug);
	}
	if ( const IConfigNode* Labels = InNode.FindChild(PadLabelsKey) )
	{
		for ( const std::string& Name : Labels->GetKeys() )
		{
			std::string Label;
			const PadButton Found = FindPadButton(Name);
			if ( Found != PadButton::Other && Labels->TryRead(Name, Label) )
			{
				Result.PadLabels[Found] = Label;
			}
			else
			{
				Result.Errors.PushBack("pad_labels: unknown pad button '" + Name + "'");
			}
		}
	}
	return Result;
}

const InputButton* InputBindings::FindButton(const InputContext InContext, const InputAction InAction, const InputDevice InDevice) const
{
	const TArray<InputButton>& Buttons = GetButtons(InContext, InAction);
	const auto Found = std::find_if(Buttons.begin(), Buttons.end(), [InDevice](const InputButton& InButton) { return InButton.Device == InDevice; });
	if ( Found != Buttons.end() )
	{
		return &*Found;
	}
	return Buttons.IsEmpty() ? nullptr : &Buttons.Front();
}

std::string InputBindings::GetLabel(const InputButton& InButton) const
{
	if ( InButton.Device == InputDevice::Gamepad )
	{
		auto* const Found = PadLabels.Find(static_cast<PadButton>(InButton.Code));
		if ( Found )
		{
			return *Found;
		}
	}
	return GetButtonLabel(InButton);
}

void InputBindings::ReadContext(const InputContext InContext, const IConfigNode& InActions)
{
	for ( const std::string& Name : InActions.GetKeys() )
	{
		const InputAction Action = FindAction(Name);
		if ( Action == InputAction::Count )
		{
			Errors.PushBack(GetContextName(InContext) + ": unknown action '" + Name + "'");
			continue;
		}
		const IConfigNode& Buttons = *InActions.FindChild(Name);
		for ( std::size_t Index = 0; Index < Buttons.GetItemCount(); ++Index )
		{
			InputButton Button;
			if ( ReadButton(*Buttons.GetItem(Index), Button) )
			{
				Bind(InContext, Action, Button);
			}
			else
			{
				Errors.PushBack(GetContextName(InContext) + "." + Name + ": button " + std::to_string(Index) + " is not a known key or pad button");
			}
		}
	}
}

bool InputBindings::ReadButton(const IConfigNode& InNode, InputButton& OutButton)
{
	const int Player = InNode.GetValueOr("player", 1);
	std::string Name;
	if ( InNode.TryRead("key", Name) )
	{
		const Key Found = FindKey(Name);
		OutButton = InputButton::FromKey(Found, Player);
		return Found != Key::Other;
	}
	if ( InNode.TryRead("pad", Name) )
	{
		const PadButton Found = FindPadButton(Name);
		OutButton = InputButton::FromPad(Found, Player);
		return Found != PadButton::Other;
	}
	return false;
}

void InputBindings::Bind(const InputContext InContext, const InputAction InAction, const InputButton& InButton)
{
	TArray<InputAction>& Actions = Contexts[IndexOf(InContext)][InButton];
	if ( std::find(Actions.begin(), Actions.end(), InAction) == Actions.end() )
	{
		Actions.PushBack(InAction);
		ButtonsOfActions[IndexOf(InContext)][static_cast<std::size_t>(InAction)].PushBack(InButton);
	}
}

const TArray<InputAction>& InputBindings::Find(const InputContext InContext, const InputButton& InButton) const
{
	static const TArray<InputAction> None;
	if ( InContext == InputContext::Count )
	{
		return None;
	}
	const auto& Map = Contexts[IndexOf(InContext)];
	auto* const Found = Map.Find(InButton);
	return Found ? *Found : None;
}

const TArray<InputButton>& InputBindings::GetButtons(const InputContext InContext, const InputAction InAction) const
{
	static const TArray<InputButton> None;
	return InContext == InputContext::Count || InAction == InputAction::Count ? None
		: ButtonsOfActions[IndexOf(InContext)][static_cast<std::size_t>(InAction)];
}

bool InputBindings::HasContext(const InputContext InContext) const
{
	return InContext != InputContext::Count && !Contexts[IndexOf(InContext)].IsEmpty();
}

const TArray<std::string>& InputBindings::GetErrors() const
{
	return Errors;
}

} // namespace game::model

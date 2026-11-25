#include "model/InputContextStack.h"

#include <algorithm>

namespace game::model
{

namespace
{
	constexpr InputContextStack::ContextId BaseId = 0;
}

InputContextStack::InputContextStack(const InputBindings& InBindings, const InputContext InBase)
	: Bindings(InBindings)
{
	Stack.push_back({ BaseId, InBase });
}

void InputContextStack::SetSink(const EventSink& InSink)
{
	Sink = InSink;
}

InputContextStack::ContextId InputContextStack::Push(const InputContext InContext)
{
	const bool bTopChanges = GetActive() != InContext;
	if ( bTopChanges )
	{
		ReleaseAll();
	}
	Stack.push_back({ NextId, InContext });
	return NextId++;
}

void InputContextStack::Remove(const ContextId InId)
{
	if ( InId == BaseId )
	{
		return;
	}
	const auto Found = std::find_if(Stack.begin(), Stack.end(), [InId](const Entry& InEntry) { return InEntry.Id == InId; });
	if ( Found == Stack.end() )
	{
		return;
	}
	const InputContext ActiveBefore = GetActive();
	Stack.erase(Found);
	if ( GetActive() != ActiveBefore )
	{
		ReleaseAll();
	}
}

InputContext InputContextStack::GetActive() const
{
	return Stack.back().Context;
}

void InputContextStack::OnButton(const InputButton& InButton, const bool bInPressed)
{
	if ( !bInPressed )
	{
		const auto Held = HeldButtons.find(InButton);
		if ( Held == HeldButtons.end() )
		{
			return;
		}
		const std::vector<InputAction> Actions = Held->second;
		HeldButtons.erase(Held);
		for ( const InputAction Action : Actions )
		{
			int& Count = HeldCount[static_cast<std::size_t>(Action)];
			Count = std::max(0, Count - 1);
			if ( Count == 0 )
			{
				Emit(Action, false);
			}
		}
		return;
	}

	if ( HeldButtons.count(InButton) )
	{
		return;
	}
	std::vector<InputAction> Actions = Bindings.Find(GetActive(), InButton);
	const std::vector<InputAction>& DebugActions = Bindings.Find(InputContext::Debug, InButton);
	Actions.insert(Actions.end(), DebugActions.begin(), DebugActions.end());
	if ( Actions.empty() )
	{
		return;
	}
	HeldButtons.emplace(InButton, Actions);
	for ( const InputAction Action : Actions )
	{
		if ( HeldCount[static_cast<std::size_t>(Action)]++ == 0 )
		{
			Emit(Action, true);
		}
	}
}

bool InputContextStack::IsHeld(const InputAction InAction) const
{
	return InAction != InputAction::Count && HeldCount[static_cast<std::size_t>(InAction)] > 0;
}

void InputContextStack::ReleaseAll()
{
	HeldButtons.clear();
	for ( std::size_t Index = 0; Index < InputActionCount; ++Index )
	{
		if ( HeldCount[Index] > 0 )
		{
			HeldCount[Index] = 0;
			Emit(static_cast<InputAction>(Index), false);
		}
	}
}

void InputContextStack::Emit(const InputAction InAction, const bool bInPressed) const
{
	if ( Sink )
	{
		Sink(InputEvent{ InAction, bInPressed });
	}
}

} // namespace game::model

#pragma once

#include <array>
#include <map>
#include <vector>

#include "model/InputBindings.h"
#include "templates/TFunction.h"

namespace game::model
{
/**
 * The active input contexts: buttons go to the context on top (and to Debug, when it is bound). A bound button raises
 * a pressed event for each of its actions, its release a released event. An action stays held while any button bound
 * to it is down, so two keys on one action do not release it early. When the top context changes (a level pauses and
 * the menu goes on top), every held action is released, so nothing stays stuck and nothing reaches the context below.
 */
class InputContextStack
{
public:
	using ContextId = int;
	using EventSink = TFunction<void(const InputEvent&)>;

	InputContextStack(const InputBindings& InBindings, InputContext InBase);

	void SetSink(const EventSink& InSink);
	ContextId Push(InputContext InContext);
	void Remove(ContextId InId);
	InputContext GetActive() const;

	void OnButton(const InputButton& InButton, bool bInPressed);
	bool IsHeld(InputAction InAction) const;
	void ReleaseAll();

private:
	struct Entry
	{
		ContextId Id = 0;
		InputContext Context = InputContext::Menu;
	};

	void Emit(InputAction InAction, bool bInPressed) const;

	const InputBindings& Bindings;
	EventSink Sink;
	std::vector<Entry> Stack;
	ContextId NextId = 1;
	std::map<InputButton, std::vector<InputAction>> HeldButtons;
	std::array<int, InputActionCount> HeldCount{};
};
} // namespace game::model

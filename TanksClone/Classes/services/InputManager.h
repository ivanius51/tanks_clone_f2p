#pragma once

#include <functional>
#include <map>
#include <string>

#include "model/InputBindings.h"
#include "model/InputContextStack.h"

namespace tanks
{

/**
 * The player's input: reads the bindings (configs/input.json), keeps the stack of input contexts and turns the
 * buttons of the input port into action events for the listeners. A scene pushes its context while it is shown;
 * pause and options push Menu, so nothing reaches gameplay below them.
 */
class InputManager
{
public:
	using ListenerId = int;
	using ActionHandler = std::function<void(const model::InputEvent&)>;

	InputManager();
	~InputManager();
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	// Reads the bindings; until then no button means anything. Called once the configs are loaded.
	void Init();

	ListenerId AddListener(const ActionHandler& InHandler);
	void RemoveListener(ListenerId InId);

	model::InputContextStack::ContextId PushContext(model::InputContext InContext);
	void RemoveContext(model::InputContextStack::ContextId InId);
	model::InputContext GetActiveContext() const;
	bool IsHeld(model::InputAction InAction) const;
	// Lets go of every held button, e.g. after a scene fade, which drops the releases that came during it.
	void ReleaseAll();

	const model::InputBindings& GetBindings() const;
	// What a HUD shows for the action's first button ("SPACE"); empty when nothing is bound.
	std::string GetButtonLabel(model::InputContext InContext, model::InputAction InAction) const;

private:
	void Dispatch(const model::InputEvent& InEvent);

	model::InputBindings Bindings;
	model::InputContextStack Contexts;
	std::map<ListenerId, ActionHandler> Listeners;
	ListenerId NextListenerId = 1;
};

} // namespace tanks

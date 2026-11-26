#pragma once

#include <map>
#include <string>

#include "model/InputBindings.h"
#include "model/InputContextStack.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"

namespace game
{
/**
 * The player's input: reads the bindings (configs/input/input.json), keeps the stack of input contexts and turns the
 * buttons of the input port into action events for the listeners. A scene pushes its context while it is shown;
 * pause and options push Menu, so nothing reaches gameplay below them.
 */
class InputManager
{
public:
	using ListenerId = int;
	using ActionHandler = TFunction<void(const model::InputEvent&), UiCallbackInlineSize>;

	InputManager();
	~InputManager();
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	void Init();

	ListenerId AddListener(const ActionHandler& InHandler);
	void RemoveListener(ListenerId InId);

	model::InputContextStack::ContextId PushContext(model::InputContext InContext);
	void RemoveContext(model::InputContextStack::ContextId InId);
	model::InputContext GetActiveContext() const;
	bool IsHeld(model::InputAction InAction) const;
	void ReleaseAll();

	const model::InputBindings& GetBindings() const;
	std::string GetButtonLabel(model::InputContext InContext, model::InputAction InAction) const;
	model::InputDevice GetLastDevice() const;

private:
	void Dispatch(const model::InputEvent& InEvent);

	model::InputBindings Bindings;
	model::InputContextStack Contexts;
	TMap<ListenerId, ActionHandler> Listeners;
	ListenerId NextListenerId = 1;
	model::InputDevice LastDevice = model::InputDevice::Keyboard;
};
} // namespace game

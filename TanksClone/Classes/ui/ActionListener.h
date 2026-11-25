#pragma once

#include "templates/TFunction.h"
#include <optional>

#include "cocos2d.h"

#include "model/InputBindings.h"
#include "model/InputContextStack.h"
#include "services/InputManager.h"

namespace game::view::cocos
{
/**
 * An invisible child node that receives input actions while its parent is on screen: it starts listening (and
 * pushes its input context, if it has one) when it enters the scene and stops when it leaves.
 */
class ActionListener final : public cocos2d::Node
{
public:
	using Handler = TFunction<void(const model::InputEvent&), 2 * UiCallbackInlineSize>;

	static ActionListener* create(const Handler& InHandler, std::optional<model::InputContext> InContext = std::nullopt);

	void onEnter() override;
	void onExit() override;

private:
	Handler OnAction;
	std::optional<model::InputContext> Context;
	InputManager::ListenerId ListenerId = 0;
	std::optional<model::InputContextStack::ContextId> PushedContext;
};

using InputActionHandler = TFunction<void(model::InputAction), UiCallbackInlineSize>;

ActionListener* AddActionHandler(cocos2d::Node* InOwner, const InputActionHandler& InOnPressed,
	std::optional<model::InputContext> InContext = std::nullopt);
} // namespace game::view::cocos

#pragma once

#include <functional>
#include <optional>

#include "cocos2d.h"

#include "model/InputBindings.h"
#include "model/InputContextStack.h"
#include "services/InputManager.h"

namespace tanks::view::cocos
{

/**
 * An invisible child node that receives input actions while its parent is on screen: it starts listening (and
 * pushes its input context, if it has one) when it enters the scene and stops when it leaves.
 */
class ActionListener final : public cocos2d::Node
{
public:
	using Handler = std::function<void(const model::InputEvent&)>;

	static ActionListener* create(const Handler& InHandler, std::optional<model::InputContext> InContext = std::nullopt);

	void onEnter() override;
	void onExit() override;

private:
	Handler OnAction;
	std::optional<model::InputContext> Context;
	InputManager::ListenerId ListenerId = 0;
	std::optional<model::InputContextStack::ContextId> PushedContext;
};

// Adds an ActionListener to InOwner; calls InOnPressed for each pressed action.
ActionListener* AddActionHandler(cocos2d::Node* InOwner, const std::function<void(model::InputAction)>& InOnPressed,
	std::optional<model::InputContext> InContext = std::nullopt);

} // namespace tanks::view::cocos

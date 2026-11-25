#include "ui/ActionListener.h"

#include "core/GameContext.h"

namespace game::view::cocos
{

ActionListener* ActionListener::create(const Handler& InHandler, const std::optional<model::InputContext> InContext)
{
	auto Result = new (std::nothrow) ActionListener();
	if ( Result && Result->init() )
	{
		Result->OnAction = InHandler;
		Result->Context = InContext;
		Result->autorelease();
		return Result;
	}
	delete Result;
	return nullptr;
}

void ActionListener::onEnter()
{
	Node::onEnter();
	InputManager& Input = GameContext::Get<InputManager>();
	if ( Context )
	{
		PushedContext = Input.PushContext(*Context);
	}
	ListenerId = Input.AddListener([this](const model::InputEvent& InEvent) { OnAction(InEvent); });
}

void ActionListener::onExit()
{
	InputManager& Input = GameContext::Get<InputManager>();
	Input.RemoveListener(ListenerId);
	if ( PushedContext )
	{
		Input.RemoveContext(*PushedContext);
		PushedContext.reset();
	}
	Node::onExit();
}

ActionListener* AddActionHandler(cocos2d::Node* const InOwner, const InputActionHandler& InOnPressed,
	const std::optional<model::InputContext> InContext)
{
	auto Listener = ActionListener::create([InOnPressed](const model::InputEvent& InEvent)
	{
		if ( InEvent.bPressed )
		{
			InOnPressed(InEvent.Action);
		}
	}, InContext);
	InOwner->addChild(Listener);
	return Listener;
}

} // namespace game::view::cocos

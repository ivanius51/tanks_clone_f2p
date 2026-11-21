#include "services/InputManager.h"

#include <vector>

#include "cocos2d.h"

#include "core/BuildInfo.h"
#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "ports/IInputSource.h"
#include "services/ResourceManager.h"

namespace tanks
{

namespace
{
	model::InputBindings LoadBindings()
	{
		const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::Input, model::ResourceKind::Config);
		model::InputBindings Result = model::InputBindings::FromConfig(GameContext::Get().GetManager<ConfigReader>().GetConfig(Path), bHasDebugTools);
		for ( const std::string& Error : Result.GetErrors() )
		{
			cocos2d::log("Input config %s: %s", Path.c_str(), Error.c_str());
		}
		return Result;
	}
}

InputManager::InputManager()
	: Contexts(Bindings, model::InputContext::Menu)
{
	Contexts.SetSink([this](const model::InputEvent& InEvent) { Dispatch(InEvent); });
	GameContext::Get().GetInputSource().SetButtonHandler([this](const model::ButtonEvent& InEvent) { Contexts.OnButton(InEvent.Button, InEvent.bPressed); });
}

void InputManager::ReleaseAll()
{
	Contexts.ReleaseAll();
}

void InputManager::Init()
{
	Contexts.ReleaseAll();
	Bindings = LoadBindings();
}

InputManager::~InputManager()
{
	GameContext::Get().GetInputSource().SetButtonHandler(nullptr);
}

InputManager::ListenerId InputManager::AddListener(const ActionHandler& InHandler)
{
	Listeners.emplace(NextListenerId, InHandler);
	return NextListenerId++;
}

void InputManager::RemoveListener(const ListenerId InId)
{
	Listeners.erase(InId);
}

model::InputContextStack::ContextId InputManager::PushContext(const model::InputContext InContext)
{
	return Contexts.Push(InContext);
}

void InputManager::RemoveContext(const model::InputContextStack::ContextId InId)
{
	Contexts.Remove(InId);
}

model::InputContext InputManager::GetActiveContext() const
{
	return Contexts.GetActive();
}

bool InputManager::IsHeld(const model::InputAction InAction) const
{
	return Contexts.IsHeld(InAction);
}

const model::InputBindings& InputManager::GetBindings() const
{
	return Bindings;
}

std::string InputManager::GetButtonLabel(const model::InputContext InContext, const model::InputAction InAction) const
{
	const std::vector<model::InputButton>& Buttons = Bindings.GetButtons(InContext, InAction);
	return Buttons.empty() ? std::string() : model::GetButtonLabel(Buttons.front());
}

void InputManager::Dispatch(const model::InputEvent& InEvent)
{
	// A handler may add or remove listeners (a scene switch, a new overlay): only those present now get this event.
	std::vector<ListenerId> Current;
	Current.reserve(Listeners.size());
	for ( const auto& Entry : Listeners )
	{
		Current.push_back(Entry.first);
	}
	for ( const ListenerId Id : Current )
	{
		const auto Found = Listeners.find(Id);
		if ( Found != Listeners.end() )
		{
			const ActionHandler Handler = Found->second;
			Handler(InEvent);
		}
	}
}

} // namespace tanks

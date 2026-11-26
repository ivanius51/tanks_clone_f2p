#include "services/InputManager.h"

#include <vector>

#include "cocos2d.h"

#include "core/BuildInfo.h"
#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "ports/IInputSource.h"
#include "services/ResourceManager.h"

namespace game
{

namespace
{
	model::InputBindings LoadBindings()
	{
		const FPersistentString Path = GameContext::Get<ResourceManager>().ResolvePath(ids::configs::Input, model::ResourceKind::Config);
		model::InputBindings Result = model::InputBindings::FromConfig(GameContext::Get<ConfigReader>().GetConfig(Path), bHasDebugTools);
		for ( const std::string& Error : Result.GetErrors() )
		{
			cocos2d::log("Input config %s: %s", std::string(Path.View()).c_str(), Error.c_str());
		}
		return Result;
	}
}

InputManager::InputManager()
	: Contexts(Bindings, model::InputContext::Menu)
{
	Contexts.SetSink([this](const model::InputEvent& InEvent) { Dispatch(InEvent); });
	GameContext::Get().GetInputSource().SetButtonHandler([this](const model::ButtonEvent& InEvent)
	{
		if ( InEvent.bPressed )
		{
			LastDevice = InEvent.Button.Device;
		}
		Contexts.OnButton(InEvent.Button, InEvent.bPressed);
	});
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
	Listeners.InsertOrAssign(NextListenerId, InHandler);
	return NextListenerId++;
}

void InputManager::RemoveListener(const ListenerId InId)
{
	Listeners.Remove(InId);
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
	const model::InputButton* const Button = Bindings.FindButton(InContext, InAction, LastDevice);
	return Button ? Bindings.GetLabel(*Button) : std::string();
}

model::InputDevice InputManager::GetLastDevice() const
{
	return LastDevice;
}

void InputManager::Dispatch(const model::InputEvent& InEvent)
{
	// A handler may add or remove listeners (a scene switch, a new overlay): only those present now get this event.
	std::vector<ListenerId> Current;
	Current.reserve(Listeners.Size());
	for ( const auto& Entry : Listeners )
	{
		Current.push_back(Entry.Key);
	}
	for ( const ListenerId Id : Current )
	{
		auto* const Found = Listeners.Find(Id);
		if ( Found )
		{
			const ActionHandler Handler = *Found;
			Handler(InEvent);
		}
	}
}

} // namespace game

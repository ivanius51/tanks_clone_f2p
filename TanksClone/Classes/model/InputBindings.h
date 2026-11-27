#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>

#include "model/ConfigNode.h"
#include "model/InputTypes.h"
#include "templates/TContainers.h"

namespace game::model
{
/** Where buttons go: only the context on top of the stack receives them (plus Debug, when it is bound). */
enum class InputContext
{
	Menu,
	Gameplay,
	Editor,
	Debug,
	Count
};

/** What a button means in a context. */
enum class InputAction
{
	UiUp,
	UiDown,
	UiConfirm,
	UiBack,
	UiLeft,
	UiRight,
	MoveUp,
	MoveDown,
	MoveLeft,
	MoveRight,
	Fire,
	Ability1,
	SwitchWeapon,
	Pause,
	SelectItem1,
	SelectItem2,
	SelectItem3,
	SelectItem4,
	SelectItem5,
	SelectItem6,
	SelectItem7,
	SelectItem8,
	SelectItem9,
	NextTab,
	PrevTab,
	Back,
	ToggleStats,
	ToggleAiDebug,
	ToggleTileDebug,
	Count
};

constexpr std::size_t InputContextCount = static_cast<std::size_t>(InputContext::Count);
constexpr std::size_t InputActionCount = static_cast<std::size_t>(InputAction::Count);

/** An action started (a bound button went down) or ended (the last button holding it went up). */
struct InputEvent
{
	InputAction Action = InputAction::UiConfirm;
	bool bPressed = false;
};

std::string GetContextName(InputContext InContext);
InputContext FindContext(std::string_view InName);
std::string GetActionName(InputAction InAction);
InputAction FindAction(std::string_view InName);

/**
 * Which buttons trigger which actions, per context (configs/input/input.json): for every context a map from a button to
 * its actions. A button may trigger several actions, an action may have several buttons.
 * JSON: { "contexts": { "<context>": { "<action>": [ { "key": "w" }, { "pad": "start", "player": 2 } ] } },
 *         "debug": { "<action>": [ ... ] }, "pad_labels": { "right_trigger": "RT" } }. Unknown names are recorded in GetErrors() and skipped.
 */
class InputBindings
{
public:
	static InputBindings FromConfig(const IConfigNode& InNode, bool bInWithDebug);

	void Bind(InputContext InContext, InputAction InAction, const InputButton& InButton);
	const TArray<InputAction>& Find(InputContext InContext, const InputButton& InButton) const;
	const TArray<InputButton>& GetButtons(InputContext InContext, InputAction InAction) const;
	const InputButton* FindButton(InputContext InContext, InputAction InAction, InputDevice InDevice) const;
	std::string GetLabel(const InputButton& InButton) const;
	bool HasContext(InputContext InContext) const;
	const TArray<std::string>& GetErrors() const;

private:
	void ReadContext(InputContext InContext, const IConfigNode& InActions);
	bool ReadButton(const IConfigNode& InNode, InputButton& OutButton);

	std::array<TMap<InputButton, TArray<InputAction>>, InputContextCount> Contexts;
	std::array<std::array<TArray<InputButton>, InputActionCount>, InputContextCount> ButtonsOfActions;
	TMap<PadButton, std::string> PadLabels;
	TArray<std::string> Errors;
};
} // namespace game::model

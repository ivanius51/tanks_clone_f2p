#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"
#include "model/InputTypes.h"

namespace tanks::model
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
	Back,
	ToggleStats,
	ToggleAiDebug,
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

// Config names: "menu", "gameplay"; "ui_up", "move_left", "select_item_3". Count when unknown.
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
	// The "debug" section is read only when bInWithDebug (builds with debug tools).
	static InputBindings FromConfig(const IConfigNode& InNode, bool bInWithDebug);

	void Bind(InputContext InContext, InputAction InAction, const InputButton& InButton);
	// Empty for a button that is not bound in the context.
	const std::vector<InputAction>& Find(InputContext InContext, const InputButton& InButton) const;
	// In config order: the HUD shows the first one.
	const std::vector<InputButton>& GetButtons(InputContext InContext, InputAction InAction) const;
	// The first button of the device bound to the action, or the first button of any device; null when unbound.
	const InputButton* FindButton(InputContext InContext, InputAction InAction, InputDevice InDevice) const;
	// What a hint shows for the button: "pad_labels" from the config ("RT"), else the button's own name.
	std::string GetLabel(const InputButton& InButton) const;
	bool HasContext(InputContext InContext) const;
	const std::vector<std::string>& GetErrors() const;

private:
	void ReadContext(InputContext InContext, const IConfigNode& InActions);
	bool ReadButton(const IConfigNode& InNode, InputButton& OutButton);

	std::array<std::map<InputButton, std::vector<InputAction>>, InputContextCount> Contexts;
	std::array<std::array<std::vector<InputButton>, InputActionCount>, InputContextCount> ButtonsOfActions;
	std::map<PadButton, std::string> PadLabels;
	std::vector<std::string> Errors;
};

} // namespace tanks::model

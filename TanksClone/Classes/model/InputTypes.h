#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace tanks::model
{

/** Keyboard keys the configs can name, independent of the engine or the platform. */
enum class Key
{
	Other,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	Up, Down, Left, Right,
	Enter, KeypadEnter, Escape, Tab, Space, Backspace, Shift, Ctrl,
	Count
};

/** Gamepad buttons the configs can name. */
enum class PadButton
{
	Other,
	A, B, X, Y,
	LeftShoulder, RightShoulder,
	Back, Start,
	DPadUp, DPadDown, DPadLeft, DPadRight,
	Count
};

enum class InputDevice
{
	Keyboard,
	Gamepad
};

/** One physical button: a key today, a gamepad button of a given player later. */
struct InputButton
{
	InputDevice Device = InputDevice::Keyboard;
	int Code = 0; // a Key for the keyboard, a PadButton for a gamepad
	int Player = 1; // 1-based

	static InputButton FromKey(Key InKey, int InPlayer = 1);
	static InputButton FromPad(PadButton InButton, int InPlayer = 1);

	bool operator==(const InputButton& InOther) const;
	bool operator<(const InputButton& InOther) const;
};

/** A button went down or up. */
struct ButtonEvent
{
	InputButton Button;
	bool bPressed = false; // false = released
};

// Config names ("w", "f3", "escape", "keypad_enter"; "start", "dpad_up"). Key::Other / PadButton::Other when unknown.
std::string GetKeyName(Key InKey);
Key FindKey(std::string_view InName);
std::string GetPadButtonName(PadButton InButton);
PadButton FindPadButton(std::string_view InName);
// What a HUD shows for a button: "SPACE", "E", "PAD START".
std::string GetButtonLabel(const InputButton& InButton);

} // namespace tanks::model

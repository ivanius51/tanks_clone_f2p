#include "model/InputTypes.h"

#include <algorithm>
#include <cctype>
#include <tuple>

namespace game::model
{

namespace
{
	static_assert(static_cast<int>(Key::Z) - static_cast<int>(Key::A) == 25, "letters are in order");
	static_assert(static_cast<int>(Key::Digit9) - static_cast<int>(Key::Digit0) == 9, "digits are in order");
	static_assert(static_cast<int>(Key::F12) - static_cast<int>(Key::F1) == 11, "function keys are in order");

	bool IsInRange(const Key InKey, const Key InFirst, const Key InLast)
	{
		return InKey >= InFirst && InKey <= InLast;
	}

	int OffsetOf(const Key InKey, const Key InFirst)
	{
		return static_cast<int>(InKey) - static_cast<int>(InFirst);
	}

	std::string ToLower(std::string_view InText)
	{
		std::string Result(InText);
		std::transform(Result.begin(), Result.end(), Result.begin(), [](const unsigned char InChar) { return static_cast<char>(std::tolower(InChar)); });
		return Result;
	}

	std::string ToUpper(std::string InText)
	{
		std::transform(InText.begin(), InText.end(), InText.begin(), [](const unsigned char InChar) { return static_cast<char>(std::toupper(InChar)); });
		return InText;
	}
}

InputButton InputButton::FromKey(const Key InKey, const int InPlayer)
{
	return { InputDevice::Keyboard, static_cast<int>(InKey), InPlayer };
}

InputButton InputButton::FromPad(const PadButton InButton, const int InPlayer)
{
	return { InputDevice::Gamepad, static_cast<int>(InButton), InPlayer };
}

bool InputButton::operator==(const InputButton& InOther) const
{
	return Device == InOther.Device && Code == InOther.Code && Player == InOther.Player;
}

bool InputButton::operator<(const InputButton& InOther) const
{
	return std::tie(Device, Player, Code) < std::tie(InOther.Device, InOther.Player, InOther.Code);
}

std::string GetKeyName(const Key InKey)
{
	if ( IsInRange(InKey, Key::A, Key::Z) )
	{
		return std::string(1, static_cast<char>('a' + OffsetOf(InKey, Key::A)));
	}
	if ( IsInRange(InKey, Key::Digit0, Key::Digit9) )
	{
		return std::string(1, static_cast<char>('0' + OffsetOf(InKey, Key::Digit0)));
	}
	if ( IsInRange(InKey, Key::F1, Key::F12) )
	{
		return "f" + std::to_string(OffsetOf(InKey, Key::F1) + 1);
	}
	switch ( InKey )
	{
		case Key::Up: return "up";
		case Key::Down: return "down";
		case Key::Left: return "left";
		case Key::Right: return "right";
		case Key::Enter: return "enter";
		case Key::KeypadEnter: return "keypad_enter";
		case Key::Escape: return "escape";
		case Key::Tab: return "tab";
		case Key::Space: return "space";
		case Key::Backspace: return "backspace";
		case Key::Shift: return "shift";
		case Key::Ctrl: return "ctrl";
		default: return "";
	}
}

Key FindKey(std::string_view InName)
{
	const std::string Name = ToLower(InName);
	for ( int Index = static_cast<int>(Key::Other) + 1; Index < static_cast<int>(Key::Count); ++Index )
	{
		if ( GetKeyName(static_cast<Key>(Index)) == Name )
		{
			return static_cast<Key>(Index);
		}
	}
	return Key::Other;
}

std::string GetPadButtonName(const PadButton InButton)
{
	switch ( InButton )
	{
		case PadButton::A: return "a";
		case PadButton::B: return "b";
		case PadButton::X: return "x";
		case PadButton::Y: return "y";
		case PadButton::LeftShoulder: return "left_shoulder";
		case PadButton::RightShoulder: return "right_shoulder";
		case PadButton::Back: return "back";
		case PadButton::Start: return "start";
		case PadButton::DPadUp: return "dpad_up";
		case PadButton::DPadDown: return "dpad_down";
		case PadButton::DPadLeft: return "dpad_left";
		case PadButton::DPadRight: return "dpad_right";
		case PadButton::LeftTrigger: return "left_trigger";
		case PadButton::RightTrigger: return "right_trigger";
		default: return "";
	}
}

PadButton FindPadButton(std::string_view InName)
{
	const std::string Name = ToLower(InName);
	for ( int Index = static_cast<int>(PadButton::Other) + 1; Index < static_cast<int>(PadButton::Count); ++Index )
	{
		if ( GetPadButtonName(static_cast<PadButton>(Index)) == Name )
		{
			return static_cast<PadButton>(Index);
		}
	}
	return PadButton::Other;
}

std::string GetButtonLabel(const InputButton& InButton)
{
	if ( InButton.Device == InputDevice::Gamepad )
	{
		return "PAD " + ToUpper(GetPadButtonName(static_cast<PadButton>(InButton.Code)));
	}
	return ToUpper(GetKeyName(static_cast<Key>(InButton.Code)));
}

} // namespace game::model

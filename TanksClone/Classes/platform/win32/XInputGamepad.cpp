#include "platform/win32/XInputGamepad.h"

#include <cstdlib>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>

#pragma comment(lib, "Xinput.lib")

namespace tanks::view::win32
{

namespace
{
	using model::PadButton;

	// Bits of our own packed state: the XInput button flags, then the stick directions and triggers.
	constexpr unsigned int StickUp = 1u << 16;
	constexpr unsigned int StickDown = 1u << 17;
	constexpr unsigned int StickLeft = 1u << 18;
	constexpr unsigned int StickRight = 1u << 19;
	constexpr unsigned int LeftTrigger = 1u << 20;
	constexpr unsigned int RightTrigger = 1u << 21;

	constexpr std::pair<unsigned int, PadButton> Buttons[] = {
		{ XINPUT_GAMEPAD_A, PadButton::A }, { XINPUT_GAMEPAD_B, PadButton::B },
		{ XINPUT_GAMEPAD_X, PadButton::X }, { XINPUT_GAMEPAD_Y, PadButton::Y },
		{ XINPUT_GAMEPAD_LEFT_SHOULDER, PadButton::LeftShoulder }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, PadButton::RightShoulder },
		{ XINPUT_GAMEPAD_BACK, PadButton::Back }, { XINPUT_GAMEPAD_START, PadButton::Start },
		{ XINPUT_GAMEPAD_DPAD_UP | StickUp, PadButton::DPadUp }, { XINPUT_GAMEPAD_DPAD_DOWN | StickDown, PadButton::DPadDown },
		{ XINPUT_GAMEPAD_DPAD_LEFT | StickLeft, PadButton::DPadLeft }, { XINPUT_GAMEPAD_DPAD_RIGHT | StickRight, PadButton::DPadRight },
		{ LeftTrigger, PadButton::LeftTrigger }, { RightTrigger, PadButton::RightTrigger },
	};

	unsigned int Pack(const XINPUT_GAMEPAD& InPad)
	{
		unsigned int Result = InPad.wButtons;
		const int DeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		// The stick picks its stronger axis only, so a diagonal never presses two directions.
		if ( std::abs(InPad.sThumbLX) >= std::abs(InPad.sThumbLY) )
		{
			Result |= InPad.sThumbLX > DeadZone ? StickRight : (InPad.sThumbLX < -DeadZone ? StickLeft : 0u);
		}
		else
		{
			Result |= InPad.sThumbLY > DeadZone ? StickUp : (InPad.sThumbLY < -DeadZone ? StickDown : 0u);
		}
		Result |= InPad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? LeftTrigger : 0u;
		Result |= InPad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? RightTrigger : 0u;
		return Result;
	}
}

void XInputGamepad::Poll(const ports::IInputSource::ButtonHandler& InHandler)
{
	for ( DWORD Index = 0; Index < Previous.size(); ++Index )
	{
		XINPUT_STATE State{};
		const unsigned int Now = XInputGetState(Index, &State) == ERROR_SUCCESS ? Pack(State.Gamepad) : 0u;
		const unsigned int Was = Previous[Index];
		Previous[Index] = Now;
		if ( Now == Was || !InHandler )
		{
			continue;
		}
		for ( const auto& [Mask, Button] : Buttons )
		{
			const bool bNow = (Now & Mask) != 0;
			if ( bNow != ((Was & Mask) != 0) )
			{
				InHandler(model::ButtonEvent{ model::InputButton::FromPad(Button, static_cast<int>(Index) + 1), bNow });
			}
		}
	}
}

} // namespace tanks::view::win32

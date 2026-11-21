#pragma once

namespace tanks::model
{

/** Keys the game reacts to, independent of the engine or the platform. */
enum class Key
{
	Other,
	Left,
	Right,
	Up,
	Down,
	A,
	D,
	W,
	S,
	E,
	Q,
	Tab,
	Space,
	Escape
};

struct KeyEvent
{
	Key Code = Key::Other;
	bool bPressed = false; // false = released
};

} // namespace tanks::model

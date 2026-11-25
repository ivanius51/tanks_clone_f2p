#include "model/AiTactic.h"

#include <array>

namespace game::model
{

namespace
{
	constexpr std::array<std::string_view, static_cast<size_t>(AiTactic::Count)> GNames =
	{
		"wander", "chase", "flank", "pin", "retreat", "guard drop", "attack base", "hunt camper", "dodge", "cover", "fake turn"
	};
	static_assert(static_cast<size_t>(AiTactic::FakeTurn) + 1 == static_cast<size_t>(AiTactic::Count), "GNames follows AiTactic");
}

std::string_view GetTacticName(const AiTactic InTactic)
{
	return InTactic < AiTactic::Count ? GNames[static_cast<size_t>(InTactic)] : std::string_view();
}

} // namespace game::model

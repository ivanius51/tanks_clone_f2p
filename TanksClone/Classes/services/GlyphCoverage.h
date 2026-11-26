#pragma once

#include <string>
#include <string_view>
#include "templates/TContainers.h"

namespace game
{
/** Finds the characters of a language's strings that its fonts cannot draw. */
class GlyphCoverage
{
public:
	static TArray<std::string> FindMissing(std::string_view InLanguageId, std::string_view InFontId);

	static void ReportAll();
};
} // namespace game

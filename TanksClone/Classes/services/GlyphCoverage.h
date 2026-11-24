#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tanks
{
/** Finds the characters of a language's strings that its fonts cannot draw. */
class GlyphCoverage
{
public:
	static std::vector<std::string> FindMissing(std::string_view InLanguageId, std::string_view InFontId);

	static void ReportAll();
};
} // namespace tanks

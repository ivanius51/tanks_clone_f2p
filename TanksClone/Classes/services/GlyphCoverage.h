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
	// Characters (UTF-8, each once) of InLanguageId's texts missing from the font behind InFontId; empty if the font cannot be read.
	static std::vector<std::string> FindMissing(std::string_view InLanguageId, std::string_view InFontId);

	// Logs the missing characters of every language for both of its fonts.
	static void ReportAll();
};

} // namespace tanks

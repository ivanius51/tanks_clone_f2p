#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tanks::model
{
/** A piece of a label text: plain text, or the name of an inline icon. */
struct TextRun
{
	bool bIcon = false;
	std::string Value;
};

/**
 * Splits a text with {icon:name} markers into runs ("Cost {icon:coin} 5" gives text, icon, text).
 * An unfinished or empty marker stays plain text. Engine-free: the view turns icon runs into sprites.
 */
std::vector<TextRun> SplitIconMarkup(std::string_view InText);
} // namespace tanks::model

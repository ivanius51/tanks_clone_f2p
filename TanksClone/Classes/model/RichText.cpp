#include "model/RichText.h"

namespace game::model
{

std::vector<TextRun> SplitIconMarkup(const std::string_view InText)
{
	constexpr std::string_view Open = "{icon:";
	std::vector<TextRun> Runs;
	std::string Plain;
	size_t Cursor = 0;
	while ( Cursor < InText.size() )
	{
		const size_t Start = InText.find(Open, Cursor);
		const size_t End = Start == std::string_view::npos ? std::string_view::npos : InText.find('}', Start);
		const bool bValid = End != std::string_view::npos && End > Start + Open.size();
		if ( !bValid )
		{
			Plain.append(InText.substr(Cursor));
			break;
		}
		Plain.append(InText.substr(Cursor, Start - Cursor));
		if ( !Plain.empty() )
		{
			Runs.push_back({ false, Plain });
			Plain.clear();
		}
		Runs.push_back({ true, std::string(InText.substr(Start + Open.size(), End - Start - Open.size())) });
		Cursor = End + 1;
	}
	if ( !Plain.empty() )
	{
		Runs.push_back({ false, Plain });
	}
	return Runs;
}

} // namespace game::model

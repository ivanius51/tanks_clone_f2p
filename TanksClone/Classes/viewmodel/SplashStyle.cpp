#include "viewmodel/SplashStyle.h"

#include "model/ConfigFields.h"

namespace tanks::viewmodel
{

SplashStyle SplashStyle::FromConfig(const model::IConfigNode* const InNode, std::vector<std::string>& OutMissing)
{
	SplashStyle Result;
	const model::ConfigFields Root(InNode, OutMissing);
	Root.Read("width", Result.Width);
	Root.Read("min_show_time", Result.MinShowTime);
	Root.Read("title", Result.Title);
	Root.Read("status", Result.Status);
	Root.Read("font_face", Result.FontFace);
	Root.Read("border_width", Result.BorderWidth);
	Root.Read("title_height_divisor", Result.TitleHeightDivisor);
	Root.Read("status_height_divisor", Result.StatusHeightDivisor);
	Root.Read("title_area_end", Result.TitleAreaEnd);
	Root.Read("status_area_end", Result.StatusAreaEnd);

	const model::ConfigFields Colors = Root.Child("colors");
	Colors.Read("background", Result.Background);
	Colors.Read("title", Result.TitleColor);
	Colors.Read("status", Result.StatusColor);
	Colors.Read("border", Result.Border);
	return Result;
}

} // namespace tanks::viewmodel

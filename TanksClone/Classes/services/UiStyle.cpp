#include "services/UiStyle.h"

#include <string>
#include <vector>

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"

namespace tanks
{

namespace
{
	constexpr const char* UiFolder = "configs/ui/";

	template <typename TStyle>
	TStyle Load(const ConfigReader& InReader, const std::string& InFile)
	{
		const std::string Id = UiFolder + InFile;
		std::vector<std::string> Missing;
		TStyle Result = TStyle::FromConfig(InReader.HasConfig(Id) ? &InReader.GetConfig(Id) : nullptr, Missing);
		for ( const std::string& Key : Missing )
		{
			cocos2d::log("UiStyle: \"%s\" is missing in %s, using the default", Key.c_str(), Id.c_str());
		}
		return Result;
	}
}

UiStyle::UiStyle()
{
	ConfigReader& Reader = GameContext::Get().GetManager<ConfigReader>();
	Reader.ReadObjectConfigs(UiFolder);
	Widgets = Load<viewmodel::WidgetStyle>(Reader, "widgets.json");
	Menu = Load<viewmodel::MenuLayout>(Reader, "menu.json");
	Hud = Load<viewmodel::HudLayout>(Reader, "hud.json");
	Editor = Load<viewmodel::EditorStyle>(Reader, "editor.json");
}

const viewmodel::WidgetStyle& UiStyle::GetWidgets() const
{
	return Widgets;
}

const viewmodel::MenuLayout& UiStyle::GetMenu() const
{
	return Menu;
}

const viewmodel::HudLayout& UiStyle::GetHud() const
{
	return Hud;
}

const viewmodel::EditorStyle& UiStyle::GetEditor() const
{
	return Editor;
}

} // namespace tanks

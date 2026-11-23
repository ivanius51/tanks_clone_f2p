#include "services/UiStyle.h"

#include <string>
#include <vector>

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"

namespace tanks
{

namespace
{
	template <typename TStyle>
	TStyle Load(const ConfigReader& InReader, const model::ResourceId& InConfig)
	{
		const std::string Id = config_refs::ResolveId(InConfig);
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
	Reader.ReadObjectConfigs(ConfigsFolder);
	Widgets = Load<viewmodel::WidgetStyle>(Reader, ids::configs::UiWidgets);
	Menu = Load<viewmodel::MenuLayout>(Reader, ids::configs::UiMenu);
	Hud = Load<viewmodel::HudLayout>(Reader, ids::configs::UiHud);
	Editor = Load<viewmodel::EditorStyle>(Reader, ids::configs::UiEditor);
	Splash = Load<viewmodel::SplashStyle>(Reader, ids::configs::UiSplash);
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

const viewmodel::SplashStyle& UiStyle::GetSplash() const
{
	return Splash;
}

} // namespace tanks

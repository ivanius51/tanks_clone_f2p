#include "services/UiStyle.h"

#include <string>

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/Log.h"
#include "services/ConfigRefs.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	template <typename TStyle>
	TStyle Load(const ConfigReader& InReader, const model::ResourceId& InConfig)
	{
		const FPersistentString Id = config_refs::ResolveId(InConfig);
		TArray<std::string> Missing;
		TStyle Result = TStyle::FromConfig(InReader.HasConfig(Id) ? &InReader.GetConfig(Id) : nullptr, Missing);
		for ( const std::string& Key : Missing )
		{
			model::Log::Info("UiStyle: \"%s\" is missing in %s, using the default", Key.c_str(), std::string(Id.View()).c_str());
		}
		return Result;
	}
}

UiStyle::UiStyle()
{
	ConfigReader& Reader = GameContext::Get<ConfigReader>();
	Reader.ReadObjectConfigs(ConfigsFolder);
	Widgets = Load<viewmodel::WidgetStyle>(Reader, ids::configs::LayoutWidgets);
	Menu = Load<viewmodel::MenuLayout>(Reader, ids::configs::UiMenu);
	Hud = Load<viewmodel::HudLayout>(Reader, ids::configs::UiHud);
	Editor = Load<viewmodel::EditorStyle>(Reader, ids::configs::UiEditor);
	Splash = Load<viewmodel::SplashStyle>(Reader, ids::configs::UiSplash);
	LoadWidgets(Reader);
	LoadAnimations(Reader);
	LoadMenus(Reader);
}

void UiStyle::LoadAnimations(const ConfigReader& InReader)
{
	const std::string_view Prefix = ids::config_groups::Animations;
	for ( const config_refs::GroupEntry& Current : config_refs::ListGroupEntries(Prefix) )
	{
		if ( !InReader.HasConfig(Current.Path) )
		{
			model::Log::Info("UiStyle: animation %s: no file %s", Current.Id.c_str(), Current.Path.c_str());
			continue;
		}
		const std::string Name = Current.Id.substr(Prefix.size());
		TArray<std::string> Missing;
		AnimationDefs[Name] = model::AnimationDef::FromConfig(InReader.GetConfig(Current.Path), Name, Missing);
		for ( const std::string& Key : Missing )
		{
			model::Log::Info("UiStyle: \"%s\" is missing in %s", Key.c_str(), Current.Path.c_str());
		}
	}
}

void UiStyle::LoadMenus(const ConfigReader& InReader)
{
	const std::string_view Prefix = ids::config_groups::Menus;
	for ( const config_refs::GroupEntry& Current : config_refs::ListGroupEntries(Prefix) )
	{
		if ( !InReader.HasConfig(Current.Path) )
		{
			model::Log::Info("UiStyle: menu %s: no file %s", Current.Id.c_str(), Current.Path.c_str());
			continue;
		}
		const std::string Name = Current.Id.substr(Prefix.size());
		MenuDefs[Name] = viewmodel::MenuDef::FromConfig(InReader.GetConfig(Current.Path), Name);
	}
}

const TMap<FPersistentString, viewmodel::MenuDef>& UiStyle::GetMenus() const
{
	return MenuDefs;
}

void UiStyle::ReplaceMenu(const viewmodel::MenuDef& InDef)
{
	MenuDefs[InDef.Id] = InDef;
}

const viewmodel::MenuDef* UiStyle::FindMenu(const std::string_view InId) const
{
	auto* const Found = MenuDefs.Find(InId);
	if ( !Found )
	{
		model::Log::Info("UiStyle: no menu \"%.*s\"", static_cast<int>(InId.size()), InId.data());
		return nullptr;
	}
	return &*Found;
}

void UiStyle::LoadWidgets(const ConfigReader& InReader)
{
	const std::string_view Prefix = ids::config_groups::Widgets;
	for ( const config_refs::GroupEntry& Current : config_refs::ListGroupEntries(Prefix) )
	{
		if ( !InReader.HasConfig(Current.Path) )
		{
			model::Log::Info("UiStyle: widget %s: no file %s", Current.Id.c_str(), Current.Path.c_str());
			continue;
		}
		const std::string Name = Current.Id.substr(Prefix.size());
		TArray<std::string> Missing;
		WidgetDefs[Name] = viewmodel::WidgetDef::FromConfig(InReader.GetConfig(Current.Path), Name, Missing, WidgetVariant);
		for ( const std::string& Key : Missing )
		{
			model::Log::Info("UiStyle: \"%s\" is missing in %s", Key.c_str(), Current.Path.c_str());
		}
	}
}

void UiStyle::SetWidgetVariant(const std::string_view InVariant)
{
	WidgetVariant = std::string(InVariant);
}

void UiStyle::ReloadWidgets()
{
	ConfigReader& Reader = GameContext::Get<ConfigReader>();
	Reader.ReadObjectConfigs(ConfigsFolder);
	LoadWidgets(Reader);
	LoadAnimations(Reader);
	LoadMenus(Reader);
}

void UiStyle::ReplaceWidget(const viewmodel::WidgetDef& InDef)
{
	WidgetDefs[InDef.Id] = InDef;
}

const model::AnimationDef* UiStyle::FindAnimation(const std::string_view InId) const
{
	auto* const Found = AnimationDefs.Find(InId);
	if ( !Found )
	{
		model::Log::Info("UiStyle: no animation \"%.*s\"", static_cast<int>(InId.size()), InId.data());
		return nullptr;
	}
	return &*Found;
}

const viewmodel::WidgetDef* UiStyle::FindWidget(const std::string_view InId) const
{
	auto* const Found = WidgetDefs.Find(InId);
	if ( !Found )
	{
		model::Log::Info("UiStyle: no widget \"%.*s\"", static_cast<int>(InId.size()), InId.data());
		return nullptr;
	}
	return &*Found;
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

} // namespace game

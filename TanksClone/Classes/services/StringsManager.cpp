#include "services/StringsManager.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"

namespace tanks
{

void StringsManager::Init()
{
	auto& Configs = GameContext::Get().GetManager<ConfigReader>();
	Strings[Language::English] = Configs.GetObjectData(config_refs::ResolveId(ids::configs::StringsEnglish));
	Strings[Language::Russian] = Configs.GetObjectData(config_refs::ResolveId(ids::configs::StringsRussian));
	Strings[Language::Ukrainian] = Configs.GetObjectData(config_refs::ResolveId(ids::configs::StringsUkrainian));
}

std::string StringsManager::GetString(const std::string& InStringId) const
{
	auto FindStringsMap = Strings.find(CurrentLanguage);
	if ( FindStringsMap == Strings.end() )
	{
		cocos2d::log("StringsManager: language strings map not found, Language - %d", static_cast<int>(CurrentLanguage));
		return InStringId;
	}

	const auto& StringsData = FindStringsMap->second.Values;
	auto FindString = StringsData.find(InStringId);
	if ( FindString == StringsData.end() )
	{
		cocos2d::log("StringsManager: string not found, StringId - %s", InStringId.c_str());
		return InStringId;
	}
	return FindString->second.asString();
}

void StringsManager::SetLanguage(const Language InLanguage)
{
	if ( CurrentLanguage != InLanguage )
	{
		CurrentLanguage = InLanguage;
		// TODO: notify the game about the language change
	}
}

} // namespace tanks

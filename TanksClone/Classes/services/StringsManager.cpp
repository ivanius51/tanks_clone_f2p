#include "services/StringsManager.h"
#include "core/ConfigReader.h"

namespace tanks
{

namespace
{
	constexpr const char* EnglishStrings = "configs/strings/english.json";
	constexpr const char* RussianStrings = "configs/strings/russian.json";
	constexpr const char* UkrainianStrings = "configs/strings/ukrainian.json";
}

StringsManager& StringsManager::GetInstance()
{
	static StringsManager Instance;
	return Instance;
}

void StringsManager::Init()
{
	auto& Configs = ConfigReader::GetInstance();
	Strings[Language::English] = Configs.GetObjectData(EnglishStrings);
	Strings[Language::Russian] = Configs.GetObjectData(RussianStrings);
	Strings[Language::Ukrainian] = Configs.GetObjectData(UkrainianStrings);
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

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/ObjectTypes.h"

namespace tanks
{
/** One line of configs/strings/languages.json. */
struct LanguageDef
{
	std::string Id;
	std::string Name;
	std::string StringsId;
	std::string DecorativeFontId;
	std::string PlainFontId;
};

/**
 * Localized strings. The languages are listed in configs/strings/languages.json (id, own name, strings file, optional fonts),
 * so a new language is data only. A missing key falls back to the default language; an unknown language id selects the default.
 */
class StringsManager
{
public:
	StringsManager() = default;
	~StringsManager() = default;
	StringsManager(const StringsManager&) = delete;
	StringsManager& operator=(const StringsManager&) = delete;

	void Init();

	std::string GetString(const std::string& InStringId) const;
	void SetLanguage(std::string_view InLanguageId);

	const std::string& GetLanguageId() const { return CurrentLanguageId; }
	const std::string& GetDefaultLanguageId() const { return DefaultLanguageId; }
	const std::vector<LanguageDef>& GetLanguages() const { return Languages; }
	const LanguageDef* FindLanguage(std::string_view InLanguageId) const;
	std::string GetNextLanguageId(std::string_view InLanguageId) const;
	const std::string& GetFontId(bool bInDecorative) const;
	std::vector<std::string> FindMissingKeys(std::string_view InLanguageId) const;
	std::vector<std::string> GetAllKeys() const;
	std::vector<std::string> GetAllTexts(std::string_view InLanguageId) const;

private:
	const cocos2d::ValueMap* FindStrings(std::string_view InLanguageId) const;

	std::vector<LanguageDef> Languages;
	std::map<std::string, ObjectConfig, std::less<>> Strings;
	std::string DefaultLanguageId;
	std::string CurrentLanguageId;
};
} // namespace tanks

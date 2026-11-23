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
	std::string Name; // written in the language itself
	std::string StringsId; // registry id of its strings file
	std::string DecorativeFontId; // empty = the default font
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

	// Call after ConfigReader has read the configs folder.
	void Init();

	// Returns the id itself if the string is not found in any language.
	std::string GetString(const std::string& InStringId) const;
	void SetLanguage(std::string_view InLanguageId);

	const std::string& GetLanguageId() const { return CurrentLanguageId; }
	const std::string& GetDefaultLanguageId() const { return DefaultLanguageId; }
	const std::vector<LanguageDef>& GetLanguages() const { return Languages; }
	const LanguageDef* FindLanguage(std::string_view InLanguageId) const;
	// The language after InLanguageId in the list (the first one after the last); the default for an unknown id.
	std::string GetNextLanguageId(std::string_view InLanguageId) const;
	// The registry id of the font for the current language; empty means the default one.
	const std::string& GetFontId(bool bInDecorative) const;
	// Keys of the default language that another language lacks (each is also logged by Init).
	std::vector<std::string> FindMissingKeys(std::string_view InLanguageId) const;
	// Every string value of a language.
	std::vector<std::string> GetAllTexts(std::string_view InLanguageId) const;

private:
	const cocos2d::ValueMap* FindStrings(std::string_view InLanguageId) const;

	std::vector<LanguageDef> Languages;
	std::map<std::string, ObjectConfig, std::less<>> Strings;
	std::string DefaultLanguageId;
	std::string CurrentLanguageId;
};

} // namespace tanks

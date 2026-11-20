#pragma once

#include <map>
#include <string>

#include "core/ObjectTypes.h"

namespace tanks
{

enum class Language
{
	None = 0,
	Russian,
	English,
	Ukrainian
};

// Localized strings, loaded from configs/strings/<language>.json.
class StringsManager
{
public:
	StringsManager() = default;
	~StringsManager() = default;
	StringsManager(const StringsManager&) = delete;
	StringsManager& operator=(const StringsManager&) = delete;

	// Call after ConfigReader has read the configs folder.
	void Init();

	// Returns the id itself if the string is not found.
	std::string GetString(const std::string& InStringId) const;
	void SetLanguage(Language InLanguage);

private:
	std::map<Language, ObjectConfig> Strings;
	Language CurrentLanguage = Language::English;
};

} // namespace tanks

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
	static StringsManager& GetInstance();

	// Call after ConfigReader has read the configs folder.
	void Init();

	// Returns the id itself if the string is not found.
	std::string GetString(const std::string& InStringId) const;
	void SetLanguage(Language InLanguage);

private:
	StringsManager() = default;
	~StringsManager() = default;
	StringsManager(const StringsManager&) = delete;
	StringsManager& operator=(const StringsManager&) = delete;

	std::map<Language, ObjectConfig> Strings;
	Language CurrentLanguage = Language::English;
};

} // namespace tanks

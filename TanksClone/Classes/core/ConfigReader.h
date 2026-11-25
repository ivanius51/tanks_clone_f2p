#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/ObjectTypes.h"
#include "templates/TStoredString.h"
#include "templates/TContainers.h"

namespace game
{
/**
 * Loads every .json file of a resources folder into memory.
 * A config is addressed by its path relative to the resources root,
 * e.g. "configs/strings/english.json".
 */
class ConfigReader
{
public:
	ConfigReader() = default;
	~ConfigReader() = default;
	ConfigReader(const ConfigReader&) = delete;
	ConfigReader& operator=(const ConfigReader&) = delete;

	const ObjectConfig& GetObjectData(std::string_view InId) const;
	const ObjectConfig& GetObjectData(const FPersistentString InId) const { return GetObjectData(InId.View()); }

	const model::IConfigNode& GetConfig(std::string_view InId) const;
	const model::IConfigNode& GetConfig(const FPersistentString InId) const { return GetConfig(InId.View()); }

	bool HasConfig(std::string_view InId) const;
	bool HasConfig(const FPersistentString InId) const { return HasConfig(InId.View()); }
	TArray<FPersistentString> GetConfigIds(std::string_view InPrefix) const;

	bool LoadConfigFile(const std::string& InFullPath, const std::string& InId);

	bool LoadConfigText(const std::string& InText, const std::string& InId);

	void ReadObjectConfigs(const std::string& InFolderPath);

private:
	TMap<FPersistentString, ObjectConfig> ConfigsData;
};
} // namespace game

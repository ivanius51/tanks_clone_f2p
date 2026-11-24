#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/ObjectTypes.h"

namespace tanks
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

	const ObjectConfig& GetObjectData(const std::string& InId) const;

	const model::IConfigNode& GetConfig(const std::string& InId) const;

	bool HasConfig(const std::string& InId) const;
	std::vector<std::string> GetConfigIds(std::string_view InPrefix) const;

	bool LoadConfigFile(const std::string& InFullPath, const std::string& InId);

	bool LoadConfigText(const std::string& InText, const std::string& InId);

	void ReadObjectConfigs(const std::string& InFolderPath);

private:
	std::map<std::string, ObjectConfig> ConfigsData;
};
} // namespace tanks

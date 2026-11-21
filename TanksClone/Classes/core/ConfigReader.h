#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "core/ObjectTypes.h"

namespace tanks
{

// Loads every .json file of a resources folder into memory.
// A config is addressed by its path relative to the resources root,
// e.g. "configs/strings/english.json".
class ConfigReader
{
public:
	ConfigReader() = default;
	~ConfigReader() = default;
	ConfigReader(const ConfigReader&) = delete;
	ConfigReader& operator=(const ConfigReader&) = delete;

	// Returns an empty config (and logs) if the id is unknown.
	const ObjectConfig& GetObjectData(const std::string& InId) const;

	// The config behind the format-free interface; an empty node (and a log) if the id is unknown.
	const model::IConfigNode& GetConfig(const std::string& InId) const;

	bool HasConfig(const std::string& InId) const;
	// Ids of all configs whose id starts with InPrefix, e.g. "configs/enemies/".
	std::vector<std::string> GetConfigIds(std::string_view InPrefix) const;

	// Parses a JSON file at an absolute path (e.g. a user-made level) and stores it under InId.
	// Returns false if the file is missing or is not a JSON object.
	bool LoadConfigFile(const std::string& InFullPath, const std::string& InId);

	// Parses JSON text (e.g. from the storage port) and stores it under InId.
	// Returns false if it is not a JSON object.
	bool LoadConfigText(const std::string& InText, const std::string& InId);

	// InFolderPath is relative to the resources root and ends with '/', e.g. "configs/".
	void ReadObjectConfigs(const std::string& InFolderPath);

private:
	std::map<std::string, ObjectConfig> ConfigsData;
};

} // namespace tanks

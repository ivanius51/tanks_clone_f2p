#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::model
{

enum class ResourceKind
{
	Sound,
	Atlas,
	Font,
	Config
};

struct ResourceEntry
{
	ResourceKind Kind = ResourceKind::Sound;
	std::string Path;
};

/**
 * Maps resource ids ("sfx.player_shot") to file paths. Read from the sections "sounds", "atlases",
 * "fonts" and "configs" of a config; an override config with the same layout replaces paths by id.
 */
class ResourceTable
{
public:
	// False when an id is listed in two sections (the first one is kept).
	bool FromConfig(const IConfigNode& InNode);
	// Replaces the path of known ids and adds new ones; returns how many entries changed.
	std::size_t ApplyOverrides(const IConfigNode& InNode);

	// nullptr when the id is unknown.
	const ResourceEntry* Find(std::string_view InId) const;
	std::vector<std::string> GetIds(ResourceKind InKind) const;
	std::vector<std::string> GetPaths(ResourceKind InKind) const;

	static const char* GetSectionName(ResourceKind InKind);

private:
	bool Set(const std::string& InId, ResourceKind InKind, const std::string& InPath);

	std::map<std::string, ResourceEntry, std::less<>> Entries;
};

} // namespace tanks::model

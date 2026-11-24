#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"
#include "model/ResourceId.h"

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
 * A section may be a path of another config instead; that config holds the same section under the same key.
 */
class ResourceTable
{
public:
	using ConfigResolver = std::function<const IConfigNode*(const std::string&)>;

	bool FromConfig(const IConfigNode& InNode, const ConfigResolver& InResolver = nullptr);
	std::size_t ApplyOverrides(const IConfigNode& InNode, const ConfigResolver& InResolver = nullptr);

	const ResourceEntry* Find(std::string_view InId) const;
	const ResourceEntry* Find(const ResourceId& InId) const;
	ResourceId FindId(std::string_view InName) const;
	std::vector<std::string> GetIds(ResourceKind InKind) const;
	std::vector<std::string> GetPaths(ResourceKind InKind) const;

	static const char* GetSectionName(ResourceKind InKind);

private:
	bool Set(const std::string& InId, ResourceKind InKind, const std::string& InPath);
	bool ReadSections(const IConfigNode& InNode, const ConfigResolver& InResolver, std::size_t& OutChanged);

	std::map<std::string, ResourceEntry, std::less<>> Entries;
};
} // namespace tanks::model

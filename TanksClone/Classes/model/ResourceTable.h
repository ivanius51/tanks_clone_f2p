#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "model/ConfigNode.h"
#include "model/ResourceId.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "templates/TStoredString.h"
#include "templates/TTemplateHelper.h"

namespace game::model
{
enum class ResourceKind
{
	Sound,
	Atlas,
	Font,
	Config,
	Count
};

struct ResourceEntry
{
	ResourceKind Kind = ResourceKind::Sound;
	FPersistentString Path;
};

/**
 * Maps resource ids ("sfx.player_shot") to file paths. Read from the sections "sounds", "atlases",
 * "fonts" and "configs" of a config; an override config with the same layout replaces paths by id.
 * A section may be a path of another config instead; that config holds the same section under the same key.
 */
class ResourceTable
{
public:
	using ConfigResolver = TFunction<const IConfigNode*(const std::string&)>;

	bool FromConfig(const IConfigNode& InNode, const ConfigResolver& InResolver = nullptr);
	std::size_t ApplyOverrides(const IConfigNode& InNode, const ConfigResolver& InResolver = nullptr);

	const ResourceEntry* Find(std::string_view InId) const;
	const ResourceEntry* Find(const ResourceId& InId) const;
	ResourceId FindId(std::string_view InName) const;
	TArray<FPersistentString> GetIds(ResourceKind InKind) const;
	TArray<FPersistentString> GetPaths(ResourceKind InKind) const;

	static const char* GetSectionName(ResourceKind InKind);

private:
	bool Set(std::string_view InId, ResourceKind InKind, std::string_view InPath);
	bool ReadSections(const IConfigNode& InNode, const ConfigResolver& InResolver, std::size_t& OutChanged);

	TMap<FPersistentString, ResourceEntry> Entries;
	TEnumArray<ResourceKind, SizeType> KindCounts{ SizeType{ 0 } };
};
} // namespace game::model

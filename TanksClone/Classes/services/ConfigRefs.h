#pragma once

#include <string>
#include <string_view>

#include "model/ConfigNode.h"
#include "model/ResourceId.h"
#include "templates/TStoredString.h"
#include "templates/TContainers.h"

namespace game
{
/** Finds configs through the id registry (configs/resources/configs.json) so that code and JSON never name a file path. */
namespace config_refs
{
FPersistentString ResolveId(const model::ResourceId& InId);

const model::IConfigNode& GetConfig(const model::ResourceId& InId);
const model::IConfigNode* FindChild(const model::ResourceId& InId, std::string_view InKey);

FPersistentString ResolveName(std::string_view InName);

/** One member of a config group: its registry id and file path. */
struct GroupEntry
{
	std::string Id;
	std::string Path;
};

TArray<GroupEntry> ListGroupEntries(std::string_view InGroupPrefix);

TArray<std::string> ListGroup(std::string_view InGroupPrefix);
} // namespace config_refs
} // namespace game

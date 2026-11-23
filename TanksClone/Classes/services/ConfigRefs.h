#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"
#include "model/ResourceId.h"

namespace tanks
{

/** Finds configs through the id registry (configs/resources/configs.json) so that code and JSON never name a file path. */
namespace config_refs
{

// The path of a config known to the code by an id from core/ResourceIds.h; empty (and logged) if the id is unknown.
std::string ResolveId(const model::ResourceId& InId);

// The parsed root of a config known by an id (an empty node, logged, if the id is unknown), and a child of it or nullptr.
const model::IConfigNode& GetConfig(const model::ResourceId& InId);
const model::IConfigNode* FindChild(const model::ResourceId& InId, std::string_view InKey);

// The path of the config that an id written in another config stands for ("cfg.bullet_mg"); empty for an empty or
// unknown id (the latter is logged).
std::string ResolveName(std::string_view InName);

/** One member of a config group: its registry id and file path. */
struct GroupEntry
{
	std::string Id;
	std::string Path;
};

// The members of a group in natural id order.
std::vector<GroupEntry> ListGroupEntries(std::string_view InGroupPrefix);

// The paths of the configs whose ids start with InGroupPrefix ("cfg.level."), in natural id order (level 2 before 10).
std::vector<std::string> ListGroup(std::string_view InGroupPrefix);

} // namespace config_refs

} // namespace tanks

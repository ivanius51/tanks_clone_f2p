#pragma once

#include <string>
#include <string_view>

#include "model/ResourceId.h"
#include "model/ResourceTable.h"

namespace tanks
{

/**
 * Turns resource ids into file paths using the id table of configs/resources.json (loaded on first use,
 * after the configs are read; its sections may sit in other config files). An unknown id or an id of another kind is logged and gives an empty path,
 * which callers treat as "no resource".
 */
class ResourceManager
{
public:
	ResourceManager() = default;
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	// The id for a name written in a config. An empty name gives an invalid id quietly (nothing set);
	// an unknown name or one of another kind is logged and also gives an invalid id.
	model::ResourceId FindId(std::string_view InName, model::ResourceKind InKind);
	std::string ResolvePath(const model::ResourceId& InId, model::ResourceKind InKind);

	const model::ResourceTable& GetTable();
	void ApplyOverrides(const model::IConfigNode& InNode);

private:
	void EnsureLoaded();

	model::ResourceTable Table;
	bool bLoaded = false;
};

} // namespace tanks

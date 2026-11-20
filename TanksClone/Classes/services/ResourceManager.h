#pragma once

#include <string>
#include <string_view>

#include "model/ResourceTable.h"

namespace tanks
{

/**
 * Turns resource ids into file paths using the id table of configs/resources.json (loaded on first use,
 * after the configs are read). An unknown id or an id of another kind is logged and gives an empty path,
 * which callers treat as "no resource".
 */
class ResourceManager
{
public:
	ResourceManager() = default;
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	std::string ResolvePath(std::string_view InId, model::ResourceKind InKind);

	const model::ResourceTable& GetTable();
	void ApplyOverrides(const model::IConfigNode& InNode);

private:
	void EnsureLoaded();

	model::ResourceTable Table;
	bool bLoaded = false;
};

} // namespace tanks

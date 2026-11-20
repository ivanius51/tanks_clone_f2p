#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "model/RefCounted.h"
#include "model/ResourceId.h"
#include "model/ResourceTable.h"
#include "services/AtlasResource.h"

namespace tanks
{

using AtlasHandle = model::Handle<AtlasResource>;

/**
 * Turns resource ids into file paths using the id table of configs/resources.json (loaded on first use,
 * after the configs are read; its sections may sit in other config files). An unknown id or an id of another kind is logged and gives an empty path,
 * which callers treat as "no resource".
 * Atlases are shared through handles: an atlas is loaded when the first handle is taken and unloaded
 * when the last one is released.
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

	// The atlas with this id name ("atlas.common"); loads it if no handle holds it yet. Empty handle on failure.
	AtlasHandle AcquireAtlas(std::string_view InName);
	// Atlases the whole game uses: kept loaded until the manager is destroyed.
	void KeepAtlasesLoaded(const std::vector<std::string>& InNames);
	bool IsAtlasLoaded(std::string_view InName) const;
	// The file an atlas was loaded from (its HD or SD path); empty if it is not loaded.
	std::string GetAtlasLoadedPath(std::string_view InName) const;
	// Loads every loaded atlas again from the texture set chosen now (HD or SD).
	void ReloadAtlases();
	// Sharp pixels (nearest filtering, the default) or smoothed ones, for every loaded and future atlas.
	void SetSmoothPixels(bool bInSmooth);
	bool IsSmoothPixels() const;

	const model::ResourceTable& GetTable();
	void ApplyOverrides(const model::IConfigNode& InNode);

private:
	void EnsureLoaded();

	model::ResourceTable Table;
	std::shared_ptr<AtlasRegistry> Atlases = std::make_shared<AtlasRegistry>();
	std::vector<AtlasHandle> KeptAtlases;
	bool bLoaded = false;
};

} // namespace tanks

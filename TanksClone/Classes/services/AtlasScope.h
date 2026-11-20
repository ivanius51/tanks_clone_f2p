#pragma once

#include <string>
#include <vector>

namespace tanks
{

/**
 * Keeps sprite atlases (.plist + .png) loaded while the scope lives, e.g. the atlases of one level.
 * Atlases are counted: when two scopes need the same atlas, it is unloaded only after the last one is gone.
 */
class AtlasScope
{
public:
	AtlasScope() = default;
	explicit AtlasScope(const std::vector<std::string>& InAtlases);
	~AtlasScope();
	AtlasScope(const AtlasScope&) = delete;
	AtlasScope& operator=(const AtlasScope&) = delete;

	// Returns false if the atlas could not be loaded.
	bool Add(const std::string& InAtlas);
	const std::vector<std::string>& GetAtlases() const;

	// For atlases the whole game uses: they stay loaded until the application ends.
	static void KeepLoaded(const std::vector<std::string>& InAtlases);
	static bool IsLoaded(const std::string& InAtlas);

	// Sharp pixels (nearest filtering, the default) or smoothed ones, for every loaded and future atlas.
	static void SetSmoothPixels(bool bInSmooth);
	static bool IsSmoothPixels();

	// Loads every loaded atlas again from the texture set chosen now (HD or SD, see DisplaySettings).
	static void ReloadAll();
	// The file an atlas was loaded from (its HD or SD path); empty if it is not loaded.
	static std::string GetLoadedPath(const std::string& InAtlas);

private:
	std::vector<std::string> Atlases;
};

} // namespace tanks

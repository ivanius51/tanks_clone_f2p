#pragma once

#include <map>
#include <string>
#include <vector>

#include "game/LevelLoader.h"
#include "services/ResourceSet.h"
#include "templates/TContainers.h"

namespace game
{
/**
 * Says which sprite atlases are needed, from configs/resources.json and the configs of the enemies:
 * the atlases the whole game uses, and the atlases of one level (its tileset and the enemies its waves can send).
 */
class ResourceManifest
{
public:
	ResourceManifest();

	const std::vector<std::string>& GetGlobalAtlases() const;
	std::string GetTilesetName(const std::string& InTileset) const;
	std::string GetTilesetAtlas(const std::string& InTileset) const;
	static std::string GetTilesetFrame(const std::string& InTileset, const std::string& InFrame);
	std::vector<std::string> GetTilesetAtlases() const;
	std::vector<std::string> GetLevelAtlases(const LevelData& InLevel) const;
	std::vector<std::string> GetEnemyAtlases() const;
	std::vector<std::string> GetLevelSounds(const LevelData& InLevel) const;
	ResourceSet GetLevelResources(const LevelData& InLevel, bool bInWithSounds) const;

private:
	std::vector<std::string> GlobalAtlases;
	TMap<FPersistentString, std::string> Tilesets;
	std::string DefaultTileset;
};
} // namespace game

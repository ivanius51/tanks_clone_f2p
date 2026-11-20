#pragma once

#include <map>
#include <string>
#include <vector>

#include "game/LevelLoader.h"
#include "services/ResourceSet.h"

namespace tanks
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
	// The tileset a level shows: the default one if the name is empty or unknown.
	std::string GetTilesetName(const std::string& InTileset) const;
	std::string GetTilesetAtlas(const std::string& InTileset) const;
	// Tilesets share frame names, so their frames carry the tileset name: "desert/wall_brick".
	static std::string GetTilesetFrame(const std::string& InTileset, const std::string& InFrame);
	std::vector<std::string> GetTilesetAtlases() const;
	std::vector<std::string> GetLevelAtlases(const LevelData& InLevel) const;
	std::vector<std::string> GetEnemyAtlases() const;
	// Sounds named by the player's tank and by the enemies the level can send.
	std::vector<std::string> GetLevelSounds(const LevelData& InLevel) const;
	// Everything a level needs loaded; without the sounds when the sound is off.
	ResourceSet GetLevelResources(const LevelData& InLevel, bool bInWithSounds) const;

private:
	std::vector<std::string> GlobalAtlases;
	std::map<std::string, std::string> Tilesets;
	std::string DefaultTileset;
};

} // namespace tanks

#pragma once

namespace tanks
{

// Name of the player-controlled object. Must match "name" in the player config.
constexpr const char* PlayerName = "PLAYER";

constexpr const char* ResourcesConfig = "configs/resources.json";
constexpr const char* ConfigsFolder = "configs/";


// The playfield is a grid of square tiles; the HUD bar sits above it.
constexpr float TileSize = 64.0f;
constexpr int GridColumns = 16;
constexpr int GridRows = 11;
// A level may be any size within these limits; GridColumns x GridRows is the size of the screen design and of a new level.
constexpr int MinMapSide = 3;
constexpr int MaxMapSide = 256;
// Screen layout, from the bottom: the player's panel, the playfield, the level bar.
constexpr float BottomHudHeight = 112.0f;
constexpr float TopHudHeight = 48.0f;

constexpr float DesignWidth = TileSize * GridColumns;
constexpr float DesignHeight = TileSize * GridRows + TopHudHeight + BottomHudHeight;

// Z orders inside the world layer
constexpr int ZBackground = 0;
constexpr int ZWall = 2;
constexpr int ZBonus = 3;
constexpr int ZTank = 5;
constexpr int ZBullet = 6;
constexpr int ZEffect = 8;

} // namespace tanks

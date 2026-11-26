#pragma once

namespace game
{
constexpr const char* PlayerName = "PLAYER";

constexpr const char* ResourcesConfig = "configs/resources.json";
constexpr const char* ConfigsFolder = "configs/";

constexpr float TileSize = 64.0f;
constexpr int GridColumns = 16;
constexpr int GridRows = 11;
constexpr int MinMapSide = 3;
constexpr int MaxMapSide = 256;
constexpr float BottomHudHeight = 112.0f;
constexpr float TopHudHeight = 48.0f;

constexpr float DesignWidth = TileSize * GridColumns;
constexpr float DesignHeight = TileSize * GridRows + TopHudHeight + BottomHudHeight;

} // namespace game

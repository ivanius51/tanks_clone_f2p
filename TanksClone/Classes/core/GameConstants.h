#pragma once

namespace tanks
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

constexpr int ZBackground = 0;
constexpr int ZWall = 2;
constexpr int ZBonus = 3;
constexpr int ZTank = 5;
constexpr int ZBullet = 6;
constexpr int ZEffect = 8;
} // namespace tanks

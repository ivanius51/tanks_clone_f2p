#pragma once

namespace tanks
{

// Name of the player-controlled object. Must match "name" in the player config.
constexpr const char* PlayerName = "PLAYER";

constexpr const char* ResourcesConfig = "configs/resources.json";
constexpr const char* ConfigsFolder = "configs/";
constexpr const char* EnemiesFolder = "configs/enemies/"; // one JSON file per enemy
// Fonts under the SIL Open Font License (fonts/OFL-*.txt), both with Ukrainian and Russian glyphs.
constexpr const char* UiFont = "fonts/PressStart2P-Regular.ttf";
constexpr const char* UiFontPlain = "fonts/PTSans-Regular.ttf";

// Object configs
constexpr const char* PlayerTankConfig = "configs/player_green_tank.json";
constexpr const char* BulletConfig = "configs/bullet.json";
constexpr const char* BonusAmmoConfig = "configs/bonus_ammo.json";
constexpr const char* BonusRepairConfig = "configs/bonus_repair.json";
constexpr const char* UpgradesConfig = "configs/upgrades.json";

// The playfield is a grid of square tiles; the HUD bar sits above it.
constexpr float TileSize = 64.0f;
constexpr int GridColumns = 16;
constexpr int GridRows = 11;
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

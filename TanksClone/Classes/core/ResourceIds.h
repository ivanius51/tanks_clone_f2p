#pragma once

#include <array>
#include <string_view>

#include "model/ResourceId.h"

namespace tanks::ids
{

namespace sounds
{
	inline constexpr model::ResourceId PlayerShot{ "sfx.player_shot" };
	inline constexpr model::ResourceId HitWall{ "sfx.hit_wall" };
	inline constexpr model::ResourceId HitArmor{ "sfx.hit_armor" };
	inline constexpr model::ResourceId EnemyExplode{ "sfx.enemy_explode" };
	inline constexpr model::ResourceId PlayerExplode{ "sfx.player_explode" };
	inline constexpr model::ResourceId BonusPickup{ "sfx.bonus_pickup" };
	inline constexpr model::ResourceId GameOver{ "sfx.game_over" };
	inline constexpr model::ResourceId LevelComplete{ "sfx.level_complete" };
	inline constexpr model::ResourceId Pause{ "sfx.pause" };
	inline constexpr model::ResourceId LevelStart{ "sfx.level_start" };
	inline constexpr model::ResourceId WallBreak{ "sfx.wall_break" };
	inline constexpr model::ResourceId BossExplode{ "sfx.boss_explode" };
	inline constexpr model::ResourceId UiClick{ "sfx.ui_click" };
	inline constexpr model::ResourceId UiMove{ "sfx.ui_move" };

	inline constexpr std::array<model::ResourceId, 14> All = {
		PlayerShot, HitWall, HitArmor, EnemyExplode, PlayerExplode, BonusPickup,
		GameOver, LevelComplete, Pause, LevelStart, WallBreak, BossExplode, UiClick, UiMove
	};
}

namespace fonts
{
	// Both fonts are under the SIL Open Font License (fonts/OFL-*.txt) and have Ukrainian and Russian glyphs.
	inline constexpr model::ResourceId Decorative{ "font.decorative" };
	inline constexpr model::ResourceId Plain{ "font.plain" };

	inline constexpr std::array<model::ResourceId, 2> All = { Decorative, Plain };
}

namespace configs
{
	inline constexpr model::ResourceId PlayerTank{ "cfg.player_tank" };
	inline constexpr model::ResourceId Bullet{ "cfg.bullet" };
	inline constexpr model::ResourceId BonusAmmo{ "cfg.bonus_ammo" };
	inline constexpr model::ResourceId BonusRepair{ "cfg.bonus_repair" };
	inline constexpr model::ResourceId Upgrades{ "cfg.upgrades" };
	inline constexpr model::ResourceId Achievements{ "cfg.achievements" };
	inline constexpr model::ResourceId DeathPenalty{ "cfg.death_penalty" };
	inline constexpr model::ResourceId Difficulty{ "cfg.difficulty" };
	inline constexpr model::ResourceId Drops{ "cfg.drops" };
	inline constexpr model::ResourceId WallBrick{ "cfg.wall_brick" };
	inline constexpr model::ResourceId WallStone{ "cfg.wall_stone" };
	inline constexpr model::ResourceId WallBase{ "cfg.wall_base" };
	inline constexpr model::ResourceId TankLook{ "cfg.tank_look" };
	inline constexpr model::ResourceId Effects{ "cfg.effects" };
	inline constexpr model::ResourceId WallWood{ "cfg.wall_wood" };
	inline constexpr model::ResourceId Input{ "cfg.input" };
	inline constexpr model::ResourceId SideHits{ "cfg.side_hits" };
	inline constexpr model::ResourceId Crate{ "cfg.crate" };
	inline constexpr model::ResourceId FuelBarrel{ "cfg.fuel_barrel" };
	inline constexpr model::ResourceId TankWreck{ "cfg.tank_wreck" };
	inline constexpr model::ResourceId LevelMine{ "cfg.level_mine" };
	inline constexpr model::ResourceId LevelWreck{ "cfg.level_wreck" };
	inline constexpr model::ResourceId Music{ "cfg.music" };
	inline constexpr model::ResourceId LevelLegend{ "cfg.level_legend" };
	inline constexpr model::ResourceId Buffs{ "cfg.buffs" };
	inline constexpr model::ResourceId Display{ "cfg.display" };
	inline constexpr model::ResourceId Background{ "cfg.background" };
	inline constexpr model::ResourceId Credits{ "cfg.credits" };
	inline constexpr model::ResourceId TankAi{ "cfg.ai.tank_ai" };
	inline constexpr model::ResourceId PlayerBot{ "cfg.ai.player_bot" };
	inline constexpr model::ResourceId EditorPalette{ "cfg.editor_palette" };
	inline constexpr model::ResourceId Languages{ "cfg.languages" };
	inline constexpr model::ResourceId LayoutWidgets{ "cfg.ui.widgets" };
	inline constexpr model::ResourceId UiMenu{ "cfg.ui.menu" };
	inline constexpr model::ResourceId UiHud{ "cfg.ui.hud" };
	inline constexpr model::ResourceId UiEditor{ "cfg.ui.editor" };
	inline constexpr model::ResourceId UiSplash{ "cfg.ui.splash" };
	inline constexpr model::ResourceId Galleries{ "cfg.test.galleries" };
	inline constexpr model::ResourceId SideHitSparks{ "cfg.side_hit_sparks" };
	inline constexpr model::ResourceId TankCorpse{ "cfg.tank_corpse" };
	inline constexpr model::ResourceId MusicRules{ "cfg.music_rules" };
	inline constexpr model::ResourceId UpgradeTree{ "cfg.upgrade_tree" };
	inline constexpr model::ResourceId BuffIcons{ "cfg.buff_icons" };

	inline constexpr std::array<model::ResourceId, 43> All = {
		PlayerTank, Bullet, BonusAmmo, BonusRepair, Upgrades, Achievements, DeathPenalty, Difficulty, Drops, WallBrick, WallStone, WallWood, WallBase, TankLook, Effects, Input, SideHits, Crate, FuelBarrel, TankWreck, LevelMine, LevelWreck, Music, LevelLegend,
		Buffs, Display, Background, Credits, TankAi, PlayerBot, EditorPalette, Languages,
		LayoutWidgets, UiMenu, UiHud, UiEditor, UiSplash, Galleries,
		SideHitSparks, TankCorpse, MusicRules, UpgradeTree, BuffIcons
	};
}

// Widget files (configs/ui/widgets/<id>.json) that code builds directly; other widget files are named by widget files.
namespace widgets
{
	inline constexpr std::string_view Button = "button";
	inline constexpr std::string_view Keycap = "keycap";
	inline constexpr std::string_view Icon = "icon";
}

// Groups of configs: every registry id with the prefix is one member (a new enemy or level is a registry line).
namespace config_groups
{
	inline constexpr std::string_view Enemies = "cfg.enemy.";
	inline constexpr std::string_view Levels = "cfg.level.";
	inline constexpr std::string_view Weapons = "cfg.weapon.";
	inline constexpr std::string_view Widgets = "cfg.widget.";
}

} // namespace tanks::ids

#pragma once

#include <array>

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

	inline constexpr std::array<model::ResourceId, 12> All = {
		PlayerShot, HitWall, HitArmor, EnemyExplode, PlayerExplode, BonusPickup,
		GameOver, LevelComplete, Pause, LevelStart, WallBreak, BossExplode
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

	inline constexpr std::array<model::ResourceId, 22> All = {
		PlayerTank, Bullet, BonusAmmo, BonusRepair, Upgrades, Achievements, DeathPenalty, Difficulty, Drops, WallBrick, WallStone, WallWood, WallBase, TankLook, Effects, Input, SideHits, Crate, FuelBarrel, TankWreck, LevelMine, LevelWreck
	};
}

} // namespace tanks::ids

#include "game/objects/TankLoader.h"

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ResourceManager.h"

namespace tanks
{

namespace
{
	constexpr const char* DefaultIcon = "icon_shoot";
	constexpr const char* DefaultName = "WEAPON_CANNON";
	constexpr const char* PrimaryAbility = "Shoot";

	model::ResourceId ReadSound(const model::IConfigNode& InNode, const char* const InKey)
	{
		std::string Name;
		return InNode.TryRead(InKey, Name) ? GameContext::Get().GetManager<ResourceManager>().FindId(Name, model::ResourceKind::Sound) : model::ResourceId();
	}
}

TankDef TankLoader::ReadTank(const model::IConfigNode& InNode)
{
	TankDef Def;
	Def.Armor = InNode.GetValueOr("armor", Def.Armor);
	Def.ReloadTime = InNode.GetValueOr("reload_time", Def.ReloadTime);
	Def.MaxAmmo = InNode.GetValueOr("max_ammo", Def.MaxAmmo);
	Def.Weapons = ReadWeapons(InNode, GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::Bullet, model::ResourceKind::Config));
	Def.Abilities = ReadAbilities(InNode);
	InNode.TryRead("buffs", Def.PermanentBuffs);
	InNode.TryRead("enemy_type", Def.EnemyKind);
	Def.ScoreReward = InNode.GetValueOr("score", Def.ScoreReward);
	Def.CoinReward = InNode.GetValueOr("coins", Def.CoinReward);
	Def.DropTable = InNode.GetValueOr("drop_table", "");
	Def.SplitInto = InNode.GetValueOr("split_into", "");
	Def.SplitCount = InNode.GetValueOr("split_count", 0);
	Def.SplitSpacingTiles = InNode.GetValueOr("split_spacing_tiles", Def.SplitSpacingTiles);
	Def.bFrontalShield = InNode.GetValueOr("frontal_shield", false);
	Def.RearDamageScale = InNode.GetValueOr("rear_damage_scale", Def.RearDamageScale);
	Def.Phases = ReadPhases(InNode);
	InNode.TryRead("ai", Def.Ai);
	Def.AiFeatures = model::AiFeatureSet::ReadSwitches(InNode);
	Def.HitSound = ReadSound(InNode, "sound_hit");
	Def.ArmorSound = ReadSound(InNode, "sound_armor");
	Def.ExplodeSound = ReadSound(InNode, "sound_explode");
	return Def;
}

void TankLoader::ReadFlash(const model::IConfigNode& InNode, WeaponDef& OutWeapon)
{
	const model::IConfigNode* const Flash = InNode.FindChild("muzzle_flash");
	if ( !Flash )
	{
		return;
	}
	OutWeapon.FlashWidth *= Flash->GetValueOr("width", 1.0f);
	OutWeapon.FlashLength *= Flash->GetValueOr("length", 1.0f);
	OutWeapon.FlashSpeed *= Flash->GetValueOr("speed", 1.0f);
	Flash->TryRead("tint", OutWeapon.FlashTint);
}

std::vector<WeaponDef> TankLoader::ReadWeapons(const model::IConfigNode& InNode, const std::string& InDefaultBullet)
{
	std::vector<WeaponDef> Weapons;
	if ( const model::IConfigNode* const List = InNode.FindChild("weapons") )
	{
		for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *List->GetItem(Index);
			WeaponDef Weapon;
			Weapon.Id = Item.GetValueOr("id", "");
			Weapon.NameKey = Item.GetValueOr("name", DefaultName);
			Weapon.Icon = Item.GetValueOr("icon", DefaultIcon);
			Weapon.BulletConfig = Item.GetValueOr("bullet", InDefaultBullet.c_str());
			Weapon.ReloadTime = Item.GetValueOr("reload_time", Weapon.ReloadTime);
			Weapon.Damage = Item.GetValueOr("damage", Weapon.Damage);
			Weapon.bUsesAmmo = Item.GetValueOr("uses_ammo", Weapon.bUsesAmmo);
			Weapon.bAutoFire = Item.GetValueOr("auto_fire", Weapon.bAutoFire);
			Weapon.HeatShots = Item.GetValueOr("heat_shots", Weapon.HeatShots);
			Weapon.CoolTime = Item.GetValueOr("cool_time", Weapon.CoolTime);
			Weapon.CoolDelay = Item.GetValueOr("cool_delay", Weapon.CoolDelay);
			Weapon.Pellets = Item.GetValueOr("pellets", Weapon.Pellets);
			Weapon.Spread = Item.GetValueOr("spread", Weapon.Spread);
			Weapon.BurstCount = Item.GetValueOr("burst", Weapon.BurstCount);
			Weapon.BurstInterval = Item.GetValueOr("burst_interval", Weapon.BurstInterval);
			Weapon.bDropsBehind = Item.GetValueOr("drops_behind", Weapon.bDropsBehind);
			Weapon.bLocked = Item.GetValueOr("locked", Weapon.bLocked);
			Weapon.bLobbed = Item.GetValueOr("lobbed", Weapon.bLobbed);
			Weapon.LobMinTiles = Item.GetValueOr("lob_min_tiles", Weapon.LobMinTiles);
			Weapon.LobMaxTiles = Item.GetValueOr("lob_max_tiles", Weapon.LobMaxTiles);
			Weapon.FireSound = ReadSound(Item, "sound");
			ReadFlash(InNode, Weapon);
			ReadFlash(Item, Weapon);
			Weapons.push_back(Weapon);
		}
	}

	if ( Weapons.empty() )
	{
		WeaponDef Cannon;
		Cannon.Id = "cannon";
		Cannon.NameKey = DefaultName;
		Cannon.Icon = DefaultIcon;
		Cannon.BulletConfig = InDefaultBullet;
		ReadFlash(InNode, Cannon);
		Weapons.push_back(Cannon);
	}
	return Weapons;
}

std::vector<TankPhaseDef> TankLoader::ReadPhases(const model::IConfigNode& InNode)
{
	std::vector<TankPhaseDef> Phases;
	if ( const model::IConfigNode* const List = InNode.FindChild("phases") )
	{
		for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *List->GetItem(Index);
			TankPhaseDef Phase;
			Phase.HealthBelow = Item.GetValueOr("health_below", Phase.HealthBelow);
			if ( Item.FindChild("abilities") )
			{
				Phase.Abilities = ReadAbilities(Item);
			}
			Item.TryRead("buffs", Phase.Buffs);
			Phase.SummonConfig = Item.GetValueOr("summon", "");
			Phase.SummonCount = Phase.SummonConfig.empty() ? 0 : Item.GetValueOr("summon_count", 1);
			Phase.SummonSpacingTiles = Item.GetValueOr("summon_spacing_tiles", Phase.SummonSpacingTiles);
			Phase.Sound = ReadSound(Item, "sound");
			Phases.push_back(Phase);
		}
	}
	return Phases;
}

std::vector<AbilityDef> TankLoader::ReadAbilities(const model::IConfigNode& InNode)
{
	std::vector<AbilityDef> Abilities;
	if ( const model::IConfigNode* const List = InNode.FindChild("abilities") )
	{
		for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *List->GetItem(Index);
			AbilityDef Ability;
			Ability.Type = Item.GetValueOr("type", "");
			Ability.BuffId = Item.GetValueOr("buff", "");
			Ability.Cooldown = Item.GetValueOr("cooldown", Ability.Cooldown);
			Ability.ActivateSound = ReadSound(Item, "sound");
			Ability.StunBuffId = Item.GetValueOr("stun_buff", "");
			Ability.Duration = Item.GetValueOr("duration", Ability.Duration);
			Ability.StunTime = Item.GetValueOr("stun_time", Ability.StunTime);
			Ability.ContactDamage = Item.GetValueOr("contact_damage", Ability.ContactDamage);
			Ability.RangeTiles = Item.GetValueOr("range_tiles", Ability.RangeTiles);
			Ability.HealAmount = Item.GetValueOr("heal", Ability.HealAmount);
			Ability.MaxTargets = Item.GetValueOr("max_targets", Ability.MaxTargets);
			Ability.BulletConfig = Item.GetValueOr("bullet", "");
			Ability.ShotDamage = Item.GetValueOr("damage", Ability.ShotDamage);
			Ability.FadeTime = Item.GetValueOr("fade_time", Ability.FadeTime);
			Ability.HiddenOpacity = Item.GetValueOr("hidden_opacity", Ability.HiddenOpacity);
			Abilities.push_back(Ability);
		}
	}
	if ( Abilities.empty() )
	{
		AbilityDef Shoot;
		Shoot.Type = PrimaryAbility;
		Abilities.push_back(Shoot);
	}
	return Abilities;
}

} // namespace tanks

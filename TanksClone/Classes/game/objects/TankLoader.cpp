#include "game/objects/TankLoader.h"

#include <algorithm>
#include <cctype>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"
#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	constexpr const char* DefaultIcon = "icon_shoot";
	constexpr const char* PrimaryAbility = "Shoot";

	model::ResourceId ReadSound(const model::IConfigNode& InNode, const char* const InKey)
	{
		std::string Name;
		return InNode.TryRead(InKey, Name) ? GameContext::Get<ResourceManager>().FindId(Name, model::ResourceKind::Sound) : model::ResourceId();
	}

	std::string DefaultWeaponName(const std::string& InId)
	{
		std::string Name = "WEAPON_" + InId;
		std::transform(Name.begin(), Name.end(), Name.begin(), [](const unsigned char InChar) { return static_cast<char>(std::toupper(InChar)); });
		return Name;
	}

	TArray<FPersistentString> ReadIds(const model::IConfigNode& InNode, const char* const InKey)
	{
		TArray<std::string> Texts;
		InNode.TryRead(InKey, Texts);
		TArray<FPersistentString> Ids;
		Ids.Reserve(Texts.Size());
		for ( const std::string& Text : Texts )
		{
			Ids.PushBack(FPersistentString{ Text });
		}
		return Ids;
	}

	std::string ReadConfigRef(const model::IConfigNode& InNode, const char* const InKey, const std::string& InDefault)
	{
		std::string Name;
		return InNode.TryRead(InKey, Name) ? std::string(config_refs::ResolveName(Name).View()) : InDefault;
	}
}

TankDef TankLoader::ReadTank(const model::IConfigNode& InNode)
{
	TankDef Def;
	Def.Armor = InNode.GetValueOr("armor", Def.Armor);
	Def.ReloadTime = InNode.GetValueOr("reload_time", Def.ReloadTime);
	Def.MaxAmmo = InNode.GetValueOr("max_ammo", Def.MaxAmmo);
	Def.Weapons = ReadWeapons(InNode, std::string(GameContext::Get<ResourceManager>().ResolvePath(ids::configs::Bullet, model::ResourceKind::Config).View()));
	Def.Abilities = ReadAbilities(InNode);
	Def.PermanentBuffs = ReadIds(InNode, "buffs");
	InNode.TryRead("enemy_type", Def.EnemyKind);
	Def.ScoreReward = InNode.GetValueOr("score", Def.ScoreReward);
	Def.CoinReward = InNode.GetValueOr("coins", Def.CoinReward);
	Def.DropTable = FPersistentString{ InNode.GetValueOr("drop_table", std::string()) };
	Def.SplitInto = FPersistentString{ ReadConfigRef(InNode, "split_into", std::string()) };
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

TArray<WeaponDef> TankLoader::ReadWeapons(const model::IConfigNode& InNode, const std::string& InDefaultBullet)
{
	TArray<WeaponDef> Weapons;
	if ( const model::IConfigNode* const List = InNode.FindChild("weapons") )
	{
		for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *List->GetItem(Index);
			WeaponDef Weapon;
			const std::optional<std::string> PlainId = Item.ReadString();
			const std::string WeaponIdText = PlainId ? *PlainId : Item.GetValueOr("id", "");
			Weapon.Id = FPersistentString{ WeaponIdText };
			// An entry that names no registered weapon is itself the definition (test fixtures).
			const std::string WeaponId = std::string(ids::config_groups::Weapons) + WeaponIdText;
			const FPersistentString WeaponPath = GameContext::Get<ResourceManager>().GetTable().Find(WeaponId) ? config_refs::ResolveName(WeaponId) : FPersistentString();
			const model::IConfigNode& Def = WeaponPath.IsEmpty() ? Item : GameContext::Get<ConfigReader>().GetConfig(WeaponPath);
			Weapon.NameKey = FPersistentString{ Def.GetValueOr("name", DefaultWeaponName(WeaponIdText)) };
			Weapon.Icon = FPersistentString{ Def.GetValueOr("icon", DefaultIcon) };
			Weapon.BulletConfig = FPersistentString{ ReadConfigRef(Def, "bullet", InDefaultBullet) };
			Weapon.ReloadTime = Def.GetValueOr("reload_time", Weapon.ReloadTime);
			Weapon.Damage = Def.GetValueOr("damage", Weapon.Damage);
			Weapon.bUsesAmmo = Def.GetValueOr("uses_ammo", Weapon.bUsesAmmo);
			Weapon.bAutoFire = Def.GetValueOr("auto_fire", Weapon.bAutoFire);
			Weapon.HeatShots = Def.GetValueOr("heat_shots", Weapon.HeatShots);
			Weapon.CoolTime = Def.GetValueOr("cool_time", Weapon.CoolTime);
			Weapon.CoolDelay = Def.GetValueOr("cool_delay", Weapon.CoolDelay);
			Weapon.Pellets = Def.GetValueOr("pellets", Weapon.Pellets);
			Weapon.Spread = Def.GetValueOr("spread", Weapon.Spread);
			Weapon.BurstCount = Def.GetValueOr("burst", Weapon.BurstCount);
			Weapon.BurstInterval = Def.GetValueOr("burst_interval", Weapon.BurstInterval);
			Weapon.bDropsBehind = Def.GetValueOr("drops_behind", Weapon.bDropsBehind);
			Weapon.bLocked = Item.GetValueOr("locked", Def.GetValueOr("locked", Weapon.bLocked));
			Weapon.bLobbed = Def.GetValueOr("lobbed", Weapon.bLobbed);
			Weapon.LobMinTiles = Def.GetValueOr("lob_min_tiles", Weapon.LobMinTiles);
			Weapon.LobMaxTiles = Def.GetValueOr("lob_max_tiles", Weapon.LobMaxTiles);
			Weapon.FireSound = ReadSound(Def, "sound");
			ReadFlash(InNode, Weapon);
			ReadFlash(Def, Weapon);
			Weapons.PushBack(Weapon);
		}
	}

	if ( Weapons.IsEmpty() )
	{
		WeaponDef Cannon;
		Cannon.Id = FPersistentString{ "cannon" };
		Cannon.NameKey = FPersistentString{ DefaultWeaponName("cannon") };
		Cannon.Icon = FPersistentString{ DefaultIcon };
		Cannon.BulletConfig = FPersistentString{ InDefaultBullet };
		ReadFlash(InNode, Cannon);
		Weapons.PushBack(Cannon);
	}
	return Weapons;
}

TArray<TankPhaseDef> TankLoader::ReadPhases(const model::IConfigNode& InNode)
{
	TArray<TankPhaseDef> Phases;
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
			Phase.Buffs = ReadIds(Item, "buffs");
			Phase.SummonConfig = FPersistentString{ ReadConfigRef(Item, "summon", std::string()) };
			Phase.SummonCount = Phase.SummonConfig.IsEmpty() ? 0 : Item.GetValueOr("summon_count", 1);
			Phase.SummonSpacingTiles = Item.GetValueOr("summon_spacing_tiles", Phase.SummonSpacingTiles);
			Phase.Sound = ReadSound(Item, "sound");
			Phases.PushBack(Phase);
		}
	}
	return Phases;
}

TArray<AbilityDef> TankLoader::ReadAbilities(const model::IConfigNode& InNode)
{
	TArray<AbilityDef> Abilities;
	if ( const model::IConfigNode* const List = InNode.FindChild("abilities") )
	{
		for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *List->GetItem(Index);
			AbilityDef Ability;
			Ability.Type = FPersistentString{ Item.GetValueOr("type", "") };
			Ability.BuffId = FPersistentString{ Item.GetValueOr("buff", "") };
			Ability.Cooldown = Item.GetValueOr("cooldown", Ability.Cooldown);
			Ability.ActivateSound = ReadSound(Item, "sound");
			Ability.StunBuffId = FPersistentString{ Item.GetValueOr("stun_buff", "") };
			Ability.Duration = Item.GetValueOr("duration", Ability.Duration);
			Ability.StunTime = Item.GetValueOr("stun_time", Ability.StunTime);
			Ability.ContactDamage = Item.GetValueOr("contact_damage", Ability.ContactDamage);
			Ability.RangeTiles = Item.GetValueOr("range_tiles", Ability.RangeTiles);
			Ability.HealAmount = Item.GetValueOr("heal", Ability.HealAmount);
			Ability.MaxTargets = Item.GetValueOr("max_targets", Ability.MaxTargets);
			Ability.BulletConfig = FPersistentString{ ReadConfigRef(Item, "bullet", std::string()) };
			Ability.ShotDamage = Item.GetValueOr("damage", Ability.ShotDamage);
			Ability.FadeTime = Item.GetValueOr("fade_time", Ability.FadeTime);
			Ability.HiddenOpacity = Item.GetValueOr("hidden_opacity", Ability.HiddenOpacity);
			Abilities.PushBack(Ability);
		}
	}
	if ( Abilities.IsEmpty() )
	{
		AbilityDef Shoot;
		Shoot.Type = FPersistentString{ PrimaryAbility };
		Abilities.PushBack(Shoot);
	}
	return Abilities;
}

} // namespace game

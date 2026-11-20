#include "game/components/Weapon.h"

#include "core/DataConverter.h"

namespace tanks
{

namespace
{
	constexpr const char* DefaultIcon = "icon_shoot";
	constexpr const char* DefaultName = "WEAPON_CANNON";
}

std::vector<WeaponDef> ReadWeapons(const cocos2d::ValueMap& InData, const std::string& InDefaultBullet)
{
	std::vector<WeaponDef> CurrentWeapons;
	auto WeaponsIt = InData.find("weapons");
	if ( WeaponsIt != InData.end() && WeaponsIt->second.getType() == cocos2d::Value::Type::VECTOR )
	{
		for ( const auto& Item : WeaponsIt->second.asValueVector() )
		{
			if ( Item.getType() != cocos2d::Value::Type::MAP )
			{
				continue;
			}
			const cocos2d::ValueMap& CurrentEntry = Item.asValueMap();
			WeaponDef Weapon;
			Weapon.Id = DataConverter::GetString(CurrentEntry, "id", "");
			Weapon.NameKey = DataConverter::GetString(CurrentEntry, "name", DefaultName);
			Weapon.Icon = DataConverter::GetString(CurrentEntry, "icon", DefaultIcon);
			Weapon.BulletConfig = DataConverter::GetString(CurrentEntry, "bullet", InDefaultBullet);
			Weapon.ReloadTime = DataConverter::GetFloat(CurrentEntry, "reload_time", 0.0f);
			Weapon.Damage = DataConverter::GetInt(CurrentEntry, "damage", 0);
			Weapon.bUsesAmmo = DataConverter::GetBool(CurrentEntry, "uses_ammo", true);
			CurrentWeapons.push_back(Weapon);
		}
	}

	if ( CurrentWeapons.empty() )
	{
		WeaponDef Cannon;
		Cannon.Id = "cannon";
		Cannon.NameKey = DefaultName;
		Cannon.Icon = DefaultIcon;
		Cannon.BulletConfig = InDefaultBullet;
		CurrentWeapons.push_back(Cannon);
	}
	return CurrentWeapons;
}

} // namespace tanks

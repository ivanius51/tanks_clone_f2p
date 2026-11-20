#include "game/components/Buff.h"

#include <algorithm>
#include <string_view>
#include "core/ConfigReader.h"
#include "core/DataConverter.h"

namespace tanks
{

namespace
{
	constexpr const char* BuffsConfig = "configs/buffs.json";
}

BuffCatalog& BuffCatalog::GetInstance()
{
	static BuffCatalog Instance;
	return Instance;
}

void BuffCatalog::Init()
{
	Buffs.clear();

	const cocos2d::ValueMap& ConfigData = ConfigReader::GetInstance().GetObjectData(BuffsConfig).Values;
	auto ListIt = ConfigData.find("buffs");
	if ( ListIt == ConfigData.end() || ListIt->second.getType() != cocos2d::Value::Type::VECTOR )
	{
		return;
	}

	for ( const auto& Item : ListIt->second.asValueVector() )
	{
		if ( Item.getType() != cocos2d::Value::Type::MAP )
		{
			continue;
		}
		const cocos2d::ValueMap& Map = Item.asValueMap();

		BuffDef CurrentDef;
		CurrentDef.Id = DataConverter::GetString(Map, "id", "");
		CurrentDef.NameKey = DataConverter::GetString(Map, "name", CurrentDef.Id);
		CurrentDef.Duration = DataConverter::GetFloat(Map, "duration", -1.0f);
		CurrentDef.bInvulnerable = DataConverter::GetBool(Map, "invulnerable", false);
		CurrentDef.Icon = DataConverter::GetString(Map, "icon", "boom3");
		auto ColorIt = Map.find("icon_color");
		if ( ColorIt != Map.end() )
		{
			CurrentDef.IconColor = DataConverter::ValueToColor3B(ColorIt->second);
		}
		auto ModifiersIt = Map.find("modifiers");
		if ( ModifiersIt != Map.end() )
		{
			CurrentDef.Modifiers = ParseStatModifiers(ModifiersIt->second);
		}
		if ( !CurrentDef.Id.empty() )
		{
			Buffs[CurrentDef.Id] = CurrentDef;
		}
	}
}

const BuffDef* BuffCatalog::Find(const std::string& InId) const
{
	auto FindIt = Buffs.find(InId);
	if ( FindIt == Buffs.end() )
	{
		CCLOGERROR("BuffCatalog: unknown buff %s", InId.c_str());
		return nullptr;
	}
	return &FindIt->second;
}

void BuffSet::Add(const BuffDef& InDef)
{
	for ( auto& CurrentActive : ActiveBuffs )
	{
		if ( CurrentActive.Def.Id == InDef.Id )
		{
			CurrentActive.Def = InDef;
			CurrentActive.Remaining = InDef.Duration;
			return;
		}
	}
	ActiveBuffs.push_back({ InDef, InDef.Duration });
}

void BuffSet::Remove(std::string_view InId)
{
	ActiveBuffs.erase(std::remove_if(ActiveBuffs.begin(), ActiveBuffs.end(),
		[&InId](const Active& InActive) { return InActive.Def.Id == InId; }), ActiveBuffs.end());
}

bool BuffSet::Update(const float InDelta)
{
	bool bExpired = false;
	for ( auto& CurrentActive : ActiveBuffs )
	{
		if ( CurrentActive.Def.Duration >= 0.0f )
		{
			CurrentActive.Remaining -= InDelta;
			bExpired = bExpired || CurrentActive.Remaining <= 0.0f;
		}
	}
	if ( bExpired )
	{
		ActiveBuffs.erase(std::remove_if(ActiveBuffs.begin(), ActiveBuffs.end(),
			[](const Active& InActive) { return InActive.Def.Duration >= 0.0f && InActive.Remaining <= 0.0f; }), ActiveBuffs.end());
	}
	return bExpired;
}

std::vector<StatModifier> BuffSet::CollectModifiers() const
{
	std::vector<StatModifier> Result;
	for ( const auto& CurrentActive : ActiveBuffs )
	{
		Result.insert(Result.end(), CurrentActive.Def.Modifiers.begin(), CurrentActive.Def.Modifiers.end());
	}
	return Result;
}

bool BuffSet::IsInvulnerable() const
{
	return std::any_of(ActiveBuffs.begin(), ActiveBuffs.end(), [](const Active& InActive) { return InActive.Def.bInvulnerable; });
}

const std::vector<BuffSet::Active>& BuffSet::GetActive() const
{
	return ActiveBuffs;
}

} // namespace tanks

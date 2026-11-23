#include "game/components/Buff.h"

#include <algorithm>

#include "ui/CocosConvert.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/components/StatsLoader.h"
#include "services/ConfigRefs.h"
#include "services/ResourceManager.h"

namespace tanks
{

void BuffCatalog::Init()
{
	Buffs.Clear();

	const model::IConfigNode* const List = config_refs::FindChild(ids::configs::Buffs, "buffs");
	if ( !List )
	{
		return;
	}

	const model::IConfigNode* const IconList = config_refs::FindChild(ids::configs::BuffIcons, "buffs");

	for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
	{
		const model::IConfigNode& Item = *List->GetItem(Index);

		BuffDef CurrentDef;
		CurrentDef.Id = Item.GetValueOr("id", "");
		CurrentDef.NameKey = Item.GetValueOr("name", CurrentDef.Id.c_str());
		CurrentDef.Duration = Item.GetValueOr("duration", -1.0f);
		CurrentDef.bInvulnerable = Item.GetValueOr("invulnerable", false);
		const model::IConfigNode* const IconNode = IconList ? IconList->FindChild(CurrentDef.Id) : nullptr;
		if ( !IconNode )
		{
			CCLOGERROR("BuffCatalog: no icon for buff %s in the buff icons config", CurrentDef.Id.c_str());
		}
		CurrentDef.Icon = IconNode ? IconNode->GetValueOr("icon", "boom3") : "boom3";
		model::Color IconColor;
		if ( IconNode && IconNode->TryRead("icon_color", IconColor) )
		{
			CurrentDef.IconColor = view::cocos::ToCocos3B(IconColor);
		}
		if ( const model::IConfigNode* const Modifiers = Item.FindChild("modifiers") )
		{
			CurrentDef.Modifiers = StatsLoader::ReadModifiers(*Modifiers);
		}
		if ( !CurrentDef.Id.empty() )
		{
			Buffs.Add(CurrentDef);
		}
	}
}

const BuffDef* BuffCatalog::Find(const std::string& InId) const
{
	const BuffDef* const Found = Buffs.Find(InId);
	if ( !Found )
	{
		CCLOGERROR("BuffCatalog: unknown buff %s", InId.c_str());
	}
	return Found;
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

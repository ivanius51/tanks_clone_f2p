#include "model/CustomLibrary.h"

#include <algorithm>
#include <optional>

namespace game::model
{

namespace
{
	constexpr size_t MaxIdLength = 40;
	constexpr std::string_view LevelIdPrefix = "custom_";
	constexpr std::string_view CampaignIdPrefix = "campaign_";

	JsonValue Strings(const std::vector<std::string>& InItems)
	{
		JsonValue::Array Items;
		for ( const std::string& Item : InItems )
		{
			Items.emplace_back(Item);
		}
		return JsonValue(std::move(Items));
	}

	std::vector<std::string> ReadStrings(const JsonValue& InArray)
	{
		std::vector<std::string> Result;
		for ( const JsonValue& Item : InArray.GetArray() )
		{
			Result.push_back(Item.AsString());
		}
		return Result;
	}
}

CustomLibrary::CustomLibrary()
{
	CampaignDef Default;
	Default.Id = std::string(DefaultCampaignId);
	Default.Name = "Custom";
	Campaigns.push_back(Default);
}

std::string CustomLibrary::LevelFileName(const std::string_view InLevelId)
{
	return "level_" + std::string(InLevelId) + ".json";
}

bool CustomLibrary::IsValidId(const std::string_view InId)
{
	return !InId.empty() && InId.size() <= MaxIdLength && std::all_of(InId.begin(), InId.end(), [](const char InCharacter)
	{
		return (InCharacter >= 'a' && InCharacter <= 'z') || (InCharacter >= '0' && InCharacter <= '9') || InCharacter == '_';
	});
}

bool CustomLibrary::HasLevel(const std::string_view InLevelId) const
{
	return std::find(LevelIds.begin(), LevelIds.end(), InLevelId) != LevelIds.end();
}

const CampaignDef* CustomLibrary::FindCampaign(const std::string_view InCampaignId) const
{
	const auto FoundIt = std::find_if(Campaigns.begin(), Campaigns.end(), [&](const CampaignDef& InCampaign) { return InCampaign.Id == InCampaignId; });
	return FoundIt != Campaigns.end() ? &*FoundIt : nullptr;
}

CampaignDef* CustomLibrary::FindCampaignMutable(const std::string_view InCampaignId)
{
	const auto FoundIt = std::find_if(Campaigns.begin(), Campaigns.end(), [&](const CampaignDef& InCampaign) { return InCampaign.Id == InCampaignId; });
	return FoundIt != Campaigns.end() ? &*FoundIt : nullptr;
}

std::string CustomLibrary::MakeLevelId() const
{
	for ( int Number = 1;; ++Number )
	{
		const std::string Id = std::string(LevelIdPrefix) + std::to_string(Number);
		if ( !HasLevel(Id) )
		{
			return Id;
		}
	}
}

bool CustomLibrary::AddLevelOnly(const std::string& InLevelId)
{
	if ( !IsValidId(InLevelId) || HasLevel(InLevelId) )
	{
		return false;
	}
	LevelIds.push_back(InLevelId);
	return true;
}

bool CustomLibrary::AddLevel(const std::string& InLevelId, const std::string_view InCampaignId)
{
	if ( !AddLevelOnly(InLevelId) )
	{
		return false;
	}
	CampaignDef* Campaign = FindCampaignMutable(InCampaignId);
	if ( !Campaign )
	{
		Campaign = FindCampaignMutable(DefaultCampaignId);
	}
	Campaign->LevelIds.push_back(InLevelId);
	return true;
}

bool CustomLibrary::RemoveLevel(const std::string_view InLevelId)
{
	const auto FoundIt = std::find(LevelIds.begin(), LevelIds.end(), InLevelId);
	if ( FoundIt == LevelIds.end() )
	{
		return false;
	}
	LevelIds.erase(FoundIt);
	for ( CampaignDef& Campaign : Campaigns )
	{
		const auto Removed = std::remove(Campaign.LevelIds.begin(), Campaign.LevelIds.end(), InLevelId);
		if ( Removed != Campaign.LevelIds.end() )
		{
			Campaign.LevelIds.erase(Removed, Campaign.LevelIds.end());
			Campaign.CompletedLevels = 0;
			Campaign.Coins = 0;
		}
	}
	return true;
}

std::string CustomLibrary::AddCampaign(const std::string& InName)
{
	int Number = 1;
	while ( FindCampaign(std::string(CampaignIdPrefix) + std::to_string(Number)) )
	{
		++Number;
	}
	CampaignDef Campaign;
	Campaign.Id = std::string(CampaignIdPrefix) + std::to_string(Number);
	Campaign.Name = InName;
	Campaigns.push_back(Campaign);
	return Campaign.Id;
}

bool CustomLibrary::RemoveCampaign(const std::string_view InCampaignId)
{
	if ( InCampaignId == DefaultCampaignId )
	{
		return false;
	}
	const auto FoundIt = std::find_if(Campaigns.begin(), Campaigns.end(), [&](const CampaignDef& InCampaign) { return InCampaign.Id == InCampaignId; });
	if ( FoundIt == Campaigns.end() )
	{
		return false;
	}
	Campaigns.erase(FoundIt);
	return true;
}

bool CustomLibrary::AssignLevel(const std::string_view InLevelId, const std::string_view InCampaignId)
{
	CampaignDef* const Target = FindCampaignMutable(InCampaignId);
	if ( !Target || !HasLevel(InLevelId) )
	{
		return false;
	}
	for ( CampaignDef& Campaign : Campaigns )
	{
		const auto Removed = std::remove(Campaign.LevelIds.begin(), Campaign.LevelIds.end(), InLevelId);
		if ( Removed != Campaign.LevelIds.end() && &Campaign != Target )
		{
			Campaign.LevelIds.erase(Removed, Campaign.LevelIds.end());
			Campaign.CompletedLevels = 0;
			Campaign.Coins = 0;
		}
	}
	if ( std::find(Target->LevelIds.begin(), Target->LevelIds.end(), InLevelId) == Target->LevelIds.end() )
	{
		Target->LevelIds.push_back(std::string(InLevelId));
	}
	return true;
}

bool CustomLibrary::SetCampaignLevels(const std::string_view InCampaignId, const std::vector<std::string>& InLevelIds)
{
	CampaignDef* const Campaign = FindCampaignMutable(InCampaignId);
	if ( !Campaign )
	{
		return false;
	}
	std::vector<std::string> Known;
	for ( const std::string& Id : InLevelIds )
	{
		if ( HasLevel(Id) )
		{
			Known.push_back(Id);
		}
	}
	if ( Known != Campaign->LevelIds )
	{
		Campaign->LevelIds = std::move(Known);
		Campaign->CompletedLevels = 0;
		Campaign->Coins = 0;
	}
	return true;
}

bool CustomLibrary::SetProgress(const std::string_view InCampaignId, const int InCompletedLevels, const int InCoins)
{
	CampaignDef* const Campaign = FindCampaignMutable(InCampaignId);
	if ( !Campaign )
	{
		return false;
	}
	Campaign->CompletedLevels = std::clamp(InCompletedLevels, 0, static_cast<int>(Campaign->LevelIds.size()));
	Campaign->Coins = std::max(0, InCoins);
	return true;
}

bool CustomLibrary::RecordWin(const std::string_view InCampaignId, const int InPosition, const int InCoins)
{
	const CampaignDef* const Campaign = FindCampaign(InCampaignId);
	if ( !Campaign || InPosition < 0 || InPosition >= static_cast<int>(Campaign->LevelIds.size()) )
	{
		return false;
	}
	return SetProgress(InCampaignId, std::max(Campaign->CompletedLevels, InPosition + 1), Campaign->Coins + std::max(0, InCoins));
}

int CustomLibrary::GetNextPosition(const std::string_view InCampaignId) const
{
	const CampaignDef* const Campaign = FindCampaign(InCampaignId);
	if ( !Campaign || Campaign->LevelIds.empty() )
	{
		return 0;
	}
	return std::min(Campaign->CompletedLevels, static_cast<int>(Campaign->LevelIds.size()) - 1);
}

JsonValue CustomLibrary::ToJson() const
{
	JsonValue::Array CampaignItems;
	for ( const CampaignDef& Campaign : Campaigns )
	{
		JsonValue::Object Members;
		Members.emplace("id", JsonValue(Campaign.Id));
		Members.emplace("name", JsonValue(Campaign.Name));
		Members.emplace("levels", Strings(Campaign.LevelIds));
		Members.emplace("completed", JsonValue(Campaign.CompletedLevels));
		Members.emplace("coins", JsonValue(Campaign.Coins));
		CampaignItems.emplace_back(std::move(Members));
	}
	JsonValue::Object Root;
	Root.emplace("levels", Strings(LevelIds));
	Root.emplace("campaigns", JsonValue(std::move(CampaignItems)));
	return JsonValue(std::move(Root));
}

bool CustomLibrary::FromJson(const JsonValue& InRoot)
{
	if ( !InRoot.IsObject() )
	{
		return false;
	}
	CustomLibrary Result;
	for ( const std::string& Id : ReadStrings(InRoot.Get("levels")) )
	{
		Result.AddLevelOnly(Id);
	}
	for ( const JsonValue& Item : InRoot.Get("campaigns").GetArray() )
	{
		const std::string Id = Item.GetString("id", "");
		const bool bIsDefault = Id == DefaultCampaignId;
		if ( !IsValidId(Id) || (Result.FindCampaign(Id) && !bIsDefault) )
		{
			continue;
		}
		if ( !bIsDefault )
		{
			CampaignDef Campaign;
			Campaign.Id = Id;
			Campaign.Name = Item.GetString("name", "");
			Result.Campaigns.push_back(Campaign);
		}
		Result.SetCampaignLevels(Id, ReadStrings(Item.Get("levels")));
		Result.SetProgress(Id, Item.GetInt("completed", 0), Item.GetInt("coins", 0));
	}
	for ( const std::string& Id : Result.LevelIds )
	{
		const bool bInCampaign = std::any_of(Result.Campaigns.begin(), Result.Campaigns.end(), [&](const CampaignDef& InCampaign)
		{
			return std::find(InCampaign.LevelIds.begin(), InCampaign.LevelIds.end(), Id) != InCampaign.LevelIds.end();
		});
		if ( !bInCampaign )
		{
			Result.FindCampaignMutable(DefaultCampaignId)->LevelIds.push_back(Id);
		}
	}
	*this = std::move(Result);
	return true;
}

bool CustomLibrary::Load(const ports::IStorage& InStorage)
{
	const std::optional<std::string> Text = InStorage.ReadText(std::string(IndexFileName));
	if ( !Text )
	{
		return false;
	}
	bool bOk = false;
	const JsonValue Root = JsonValue::Parse(*Text, &bOk);
	return bOk && FromJson(Root);
}

bool CustomLibrary::Save(ports::IStorage& InStorage) const
{
	return InStorage.WriteText(std::string(IndexFileName), ToJson().ToString());
}

} // namespace game::model

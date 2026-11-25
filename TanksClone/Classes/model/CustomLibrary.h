#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/JsonValue.h"
#include "ports/IStorage.h"

namespace game::model
{
/** A player-made campaign: an ordered list of custom level ids with its own progress and coins. */
struct CampaignDef
{
	std::string Id;
	std::string Name;
	std::vector<std::string> LevelIds;
	int CompletedLevels = 0;
	int Coins = 0;
};

/**
 * What the player made: custom levels (each saved as its own file through the storage port) and custom campaigns
 * built from them. The index lives in one small file, `custom_library.json`, so no folder listing is needed.
 */
class CustomLibrary
{
public:
	static constexpr std::string_view IndexFileName = "custom_library.json";
	static constexpr std::string_view DefaultCampaignId = "custom";

	CustomLibrary();

	static std::string LevelFileName(std::string_view InLevelId);
	static bool IsValidId(std::string_view InId);

	const std::vector<std::string>& GetLevelIds() const { return LevelIds; }
	const std::vector<CampaignDef>& GetCampaigns() const { return Campaigns; }
	bool HasLevel(std::string_view InLevelId) const;
	const CampaignDef* FindCampaign(std::string_view InCampaignId) const;

	std::string MakeLevelId() const;
	bool AddLevel(const std::string& InLevelId, std::string_view InCampaignId = DefaultCampaignId);
	bool RemoveLevel(std::string_view InLevelId);

	std::string AddCampaign(const std::string& InName);
	bool RemoveCampaign(std::string_view InCampaignId);
	bool AssignLevel(std::string_view InLevelId, std::string_view InCampaignId);
	bool SetCampaignLevels(std::string_view InCampaignId, const std::vector<std::string>& InLevelIds);
	bool SetProgress(std::string_view InCampaignId, int InCompletedLevels, int InCoins);
	bool RecordWin(std::string_view InCampaignId, int InPosition, int InCoins);
	int GetNextPosition(std::string_view InCampaignId) const;

	JsonValue ToJson() const;
	bool FromJson(const JsonValue& InRoot);

	bool Load(const ports::IStorage& InStorage);
	bool Save(ports::IStorage& InStorage) const;

private:
	CampaignDef* FindCampaignMutable(std::string_view InCampaignId);
	bool AddLevelOnly(const std::string& InLevelId);

	std::vector<std::string> LevelIds;
	std::vector<CampaignDef> Campaigns;
};
} // namespace game::model

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/JsonValue.h"
#include "ports/IStorage.h"

namespace tanks::model
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

	// The storage file of a level.
	static std::string LevelFileName(std::string_view InLevelId);
	// Lowercase letters, digits and underscore, 1 to 40 characters.
	static bool IsValidId(std::string_view InId);

	const std::vector<std::string>& GetLevelIds() const { return LevelIds; }
	const std::vector<CampaignDef>& GetCampaigns() const { return Campaigns; }
	bool HasLevel(std::string_view InLevelId) const;
	const CampaignDef* FindCampaign(std::string_view InCampaignId) const;

	// The first "custom_N" not used by a level.
	std::string MakeLevelId() const;
	// False for an invalid or repeated id.
	bool AddLevel(const std::string& InLevelId);
	// Also takes the level out of every campaign.
	bool RemoveLevel(std::string_view InLevelId);

	// Returns the id of the new campaign ("campaign_N").
	std::string AddCampaign(const std::string& InName);
	bool RemoveCampaign(std::string_view InCampaignId);
	// The order and content of the campaign; ids of unknown levels are dropped. Progress is reset when the list changes.
	bool SetCampaignLevels(std::string_view InCampaignId, const std::vector<std::string>& InLevelIds);
	// Clamped to the campaign length; coins never go below 0.
	bool SetProgress(std::string_view InCampaignId, int InCompletedLevels, int InCoins);
	// A won level at this position (0 = the first): progress only moves forward and the coins add up.
	bool RecordWin(std::string_view InCampaignId, int InPosition, int InCoins);
	// The level to play next: the first one not completed, or the last one when all are done.
	int GetNextPosition(std::string_view InCampaignId) const;

	JsonValue ToJson() const;
	// Entries that make no sense (bad or repeated ids) are skipped. False when the text is not a JSON object.
	bool FromJson(const JsonValue& InRoot);

	// False when the index file is missing or broken (the library is then left unchanged).
	bool Load(const ports::IStorage& InStorage);
	bool Save(ports::IStorage& InStorage) const;

private:
	CampaignDef* FindCampaignMutable(std::string_view InCampaignId);

	std::vector<std::string> LevelIds;
	std::vector<CampaignDef> Campaigns;
};

} // namespace tanks::model

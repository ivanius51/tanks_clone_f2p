#include "game/ai/AiSettingsLoader.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/JsonConfigNode.h"
#include "core/ResourceIds.h"
#include "services/ResourceManager.h"

namespace game
{

namespace
{
	const model::IConfigNode& ConfigOf(const model::ResourceId& InId)
	{
		ResourceManager& Resources = GameContext::Get<ResourceManager>();
		return GameContext::Get<ConfigReader>().GetConfig(Resources.ResolvePath(InId, model::ResourceKind::Config));
	}

	const char* ProfileName(const EAiType InType)
	{
		switch ( InType )
		{
			case EAiType::Wander:
				return "Wander";
			case EAiType::Hunter:
				return "Hunter";
			default:
				return "None";
		}
	}

	const model::IConfigNode& EmptyNode()
	{
		static const JsonConfigNode Empty(model::JsonValue(), "<missing>");
		return Empty;
	}

	const model::IConfigNode& ChildOrEmpty(const model::IConfigNode* const InNode, const char* const InKey)
	{
		const model::IConfigNode* const Child = InNode ? InNode->FindChild(InKey) : nullptr;
		return Child ? *Child : EmptyNode();
	}
}

TankAiSettings AiSettingsLoader::LoadTankAi(const EAiType InType)
{
	const model::IConfigNode& Root = ConfigOf(ids::configs::TankAi);
	return ReadTankAi(ChildOrEmpty(Root.FindChild("profiles"), ProfileName(InType)));
}

PlayerBotSettings AiSettingsLoader::LoadPlayerBot()
{
	return ReadPlayerBot(ConfigOf(ids::configs::PlayerBot));
}

TankAiSettings AiSettingsLoader::ReadTankAi(const model::IConfigNode& InNode)
{
	TankAiSettings Settings;
	const model::Vec2f ThinkInterval = InNode.GetValue<model::Vec2f>("think_interval");
	Settings.SetThinkInterval(ThinkInterval.X, ThinkInterval.Y);
	Settings.SetFocusOnPlayerChance(InNode.GetValue<float>("focus_on_player_chance"));
	Settings.SetRandomFireRate(InNode.GetValue<float>("random_fire_rate"));
	Settings.SetSecondaryAbilityRate(InNode.GetValue<float>("secondary_ability_rate"));
	Settings.SetAimTolerance(InNode.GetValue<float>("aim_tolerance_tiles") * TileSize);
	Settings.SetCellCenterTolerance(InNode.GetValue<float>("cell_center_tolerance"));
	Settings.SetMaxTurnWait(InNode.GetValue<float>("max_turn_wait"));
	Settings.SetFeatures(model::AiFeatureSet::ReadSwitches(InNode));
	Settings.SetBaseFocusChance(InNode.GetValueOr("base_focus_chance", 0.0f));
	Settings.SetCamp(InNode.GetValueOr("camp_radius_tiles", 0.0f) * TileSize, InNode.GetValueOr("camp_time", 0.0f));
	Settings.SetFakeTurn(InNode.GetValueOr("fake_turn_chance", 0.0f), InNode.GetValueOr("fake_turn_time", 0.0f));
	Settings.SetGuard(InNode.GetValueOr("guard_range_tiles", 0.0f) * TileSize, InNode.GetValueOr("guard_radius_tiles", 0.0f) * TileSize);
	Settings.SetRetreatHealth(InNode.GetValueOr("retreat_health", 0.0f));
	Settings.SetDodgeChance(InNode.GetValueOr("dodge_chance", 0.0f));
	Settings.SetPathBreakCost(InNode.GetValue<int>("path_break_cost"));
	Settings.SetClearWay(InNode.GetValue<float>("clear_way_rate"), InNode.GetValue<float>("clear_way_tiles"));
	Settings.SetDodgeLookahead(InNode.GetValueOr("dodge_lookahead", 0.0f));
	Settings.SetFlankChance(InNode.GetValueOr("flank_chance", 0.0f));
	return Settings;
}

PlayerBotSettings AiSettingsLoader::ReadPlayerBot(const model::IConfigNode& InNode)
{
	PlayerBotSettings Settings;
	Settings.SetThinkInterval(InNode.GetValue<float>("think_interval"));
	Settings.SetAimTolerance(InNode.GetValue<float>("aim_tolerance_tiles") * TileSize);
	Settings.SetAbilityRange(InNode.GetValue<float>("ability_range_tiles") * TileSize);
	Settings.SetHoldDistance(InNode.GetValue<float>("hold_distance_tiles") * TileSize);
	Settings.SetStuckTime(InNode.GetValue<float>("stuck_time"));
	Settings.SetHomeHeight(InNode.GetValue<float>("home_rows") * TileSize);
	Settings.SetFacingAlignment(InNode.GetValue<float>("facing_alignment"));
	return Settings;
}

} // namespace game

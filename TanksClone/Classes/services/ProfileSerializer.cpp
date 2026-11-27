#include "services/ProfileSerializer.h"

#include "model/JsonValue.h"
#include "model/TextSize.h"
#include "services/DisplaySettings.h"

namespace game
{

std::string ProfileSerializer::ToJson(const PlayerProfile& InProfile)
{
	model::JsonValue::Object Root;
	Root.InsertOrAssign("coins", model::JsonValue(InProfile.Coins));
	Root.InsertOrAssign("best_score", model::JsonValue(InProfile.BestScore));
	Root.InsertOrAssign("levels_completed", model::JsonValue(InProfile.LevelsCompleted));
	Root.InsertOrAssign("sound", model::JsonValue(InProfile.bSoundEnabled));
	Root.InsertOrAssign("fps", model::JsonValue(InProfile.FpsLimit));
	Root.InsertOrAssign("language", model::JsonValue(InProfile.CurrentLanguage));
	Root.InsertOrAssign("vsync", model::JsonValue(InProfile.bVerticalSync));
	Root.InsertOrAssign("window_mode", model::JsonValue(static_cast<int>(InProfile.CurrentWindowMode)));
	Root.InsertOrAssign("map_scale", model::JsonValue(InProfile.MapScale));
	Root.InsertOrAssign("smooth_pixels", model::JsonValue(InProfile.bSmoothPixels));
	Root.InsertOrAssign("smooth_walls", model::JsonValue(InProfile.bSmoothWalls));
	Root.InsertOrAssign("tilted_tanks", model::JsonValue(InProfile.bTiltedTanks));
	Root.InsertOrAssign("graphics_quality", model::JsonValue(InProfile.GraphicsQualityLevel));
	Root.InsertOrAssign("hd_textures", model::JsonValue(InProfile.bHdTextures));
	if ( InProfile.WindowPosition )
	{
		Root.InsertOrAssign("window_x", model::JsonValue(InProfile.WindowPosition.Get().X));
		Root.InsertOrAssign("window_y", model::JsonValue(InProfile.WindowPosition.Get().Y));
	}
	Root.InsertOrAssign("effects_volume", model::JsonValue(InProfile.EffectsVolume));
	Root.InsertOrAssign("music_volume", model::JsonValue(InProfile.MusicVolume));
	Root.InsertOrAssign("audio_device", model::JsonValue(InProfile.AudioDevice));
	Root.InsertOrAssign("text_size", model::JsonValue(static_cast<int>(InProfile.CurrentTextSize)));

	model::JsonValue::Object Upgrades;
	for ( const auto& Upgrade : InProfile.Upgrades )
	{
		Upgrades.InsertOrAssign(Upgrade.Key.View(), model::JsonValue(Upgrade.Value));
	}
	Root.InsertOrAssign("upgrades", model::JsonValue(std::move(Upgrades)));
	model::JsonValue::Object BestScores;
	for ( const auto& Best : InProfile.BestScores )
	{
		BestScores.InsertOrAssign(Best.Key.View(), model::JsonValue(Best.Value));
	}
	Root.InsertOrAssign("best_scores", model::JsonValue(std::move(BestScores)));

	model::JsonValue::Object Counters;
	for ( const auto& Counter : InProfile.Counters )
	{
		Counters.InsertOrAssign(Counter.Key.View(), model::JsonValue(Counter.Value));
	}
	Root.InsertOrAssign("counters", model::JsonValue(std::move(Counters)));

	model::JsonValue::Object Achievements;
	for ( const FPersistentString Id : InProfile.Achievements )
	{
		Achievements.InsertOrAssign(Id.View(), model::JsonValue(true));
	}
	Root.InsertOrAssign("achievements", model::JsonValue(std::move(Achievements)));
	Root.InsertOrAssign("difficulty", model::JsonValue(InProfile.Difficulty));

	model::JsonValue::Object LevelDeaths;
	for ( const auto& Deaths : InProfile.LevelDeaths )
	{
		LevelDeaths.InsertOrAssign(std::to_string(Deaths.Key), model::JsonValue(Deaths.Value));
	}
	Root.InsertOrAssign("level_deaths", model::JsonValue(std::move(LevelDeaths)));
	return model::JsonValue(std::move(Root)).ToString();
}

PlayerProfile ProfileSerializer::FromJson(const model::IConfigNode& InNode)
{
	PlayerProfile Profile;
	Profile.Coins = InNode.GetValueOr("coins", Profile.Coins);
	Profile.BestScore = InNode.GetValueOr("best_score", Profile.BestScore);
	Profile.LevelsCompleted = InNode.GetValueOr("levels_completed", Profile.LevelsCompleted);
	Profile.bSoundEnabled = InNode.GetValueOr("sound", Profile.bSoundEnabled);
	Profile.FpsLimit = InNode.GetValueOr("fps", Profile.FpsLimit);
	Profile.bVerticalSync = InNode.GetValueOr("vsync", Profile.bVerticalSync);
	Profile.CurrentWindowMode = InNode.GetValueOr("window_mode", 0) == static_cast<int>(EWindowMode::Borderless) ? EWindowMode::Borderless : EWindowMode::Windowed;
	Profile.MapScale = InNode.GetValueOr("map_scale", Profile.MapScale);
	Profile.bSmoothPixels = InNode.GetValueOr("smooth_pixels", Profile.bSmoothPixels);
	Profile.bSmoothWalls = InNode.GetValueOr("smooth_walls", Profile.bSmoothWalls);
	Profile.bTiltedTanks = InNode.GetValueOr("tilted_tanks", Profile.bTiltedTanks);
	Profile.GraphicsQualityLevel = InNode.GetValueOr("graphics_quality", Profile.GraphicsQualityLevel);
	Profile.bHdTextures = InNode.GetValueOr("hd_textures", Profile.bHdTextures);
	if ( InNode.Has("window_x") && InNode.Has("window_y") )
	{
		Profile.WindowPosition = PixelPoint{ InNode.GetValueOr("window_x", 0), InNode.GetValueOr("window_y", 0) };
	}
	Profile.EffectsVolume = InNode.GetValueOr("effects_volume", Profile.EffectsVolume);
	Profile.MusicVolume = InNode.GetValueOr("music_volume", Profile.MusicVolume);
	Profile.AudioDevice = InNode.GetValueOr("audio_device", Profile.AudioDevice);
	Profile.CurrentTextSize = model::TextScales::FromIndex(InNode.GetValueOr("text_size", static_cast<int>(Profile.CurrentTextSize)));

	if ( !InNode.TryRead("language", Profile.CurrentLanguage) )
	{
		// Old profiles stored the language as a number (1 Russian, 3 Ukrainian).
		const int LegacyLanguage = InNode.GetValueOr("language", 0);
		Profile.CurrentLanguage = LegacyLanguage == 1 ? "ru" : (LegacyLanguage == 3 ? "uk" : "");
	}

	if ( const model::IConfigNode* const BestScores = InNode.FindChild("best_scores") )
	{
		for ( const std::string& Id : BestScores->GetKeys() )
		{
			Profile.BestScores[Id] = BestScores->GetValueOr(Id, 0);
		}
	}
	if ( const model::IConfigNode* const Upgrades = InNode.FindChild("upgrades") )
	{
		for ( const std::string& Id : Upgrades->GetKeys() )
		{
			Profile.Upgrades[Id] = Upgrades->GetValueOr(Id, 0);
		}
	}
	if ( const model::IConfigNode* const Counters = InNode.FindChild("counters") )
	{
		for ( const std::string& Id : Counters->GetKeys() )
		{
			Profile.Counters[Id] = Counters->GetValueOr(Id, 0);
		}
	}
	if ( const model::IConfigNode* const Achievements = InNode.FindChild("achievements") )
	{
		for ( const std::string& Id : Achievements->GetKeys() )
		{
			Profile.Achievements.Insert(Id);
		}
	}
	Profile.Difficulty = InNode.GetValueOr("difficulty", Profile.Difficulty.c_str());
	if ( const model::IConfigNode* const LevelDeaths = InNode.FindChild("level_deaths") )
	{
		for ( const std::string& Level : LevelDeaths->GetKeys() )
		{
			Profile.LevelDeaths[std::atoi(Level.c_str())] = LevelDeaths->GetValueOr(Level, 0);
		}
	}
	return Profile;
}

} // namespace game

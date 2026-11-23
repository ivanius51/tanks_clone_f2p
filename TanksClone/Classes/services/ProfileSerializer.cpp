#include "services/ProfileSerializer.h"

#include "model/JsonValue.h"

namespace tanks
{

std::string ProfileSerializer::ToJson(const PlayerProfile& InProfile)
{
	model::JsonValue::Object Root;
	Root.emplace("coins", model::JsonValue(InProfile.Coins));
	Root.emplace("best_score", model::JsonValue(InProfile.BestScore));
	Root.emplace("levels_completed", model::JsonValue(InProfile.LevelsCompleted));
	Root.emplace("sound", model::JsonValue(InProfile.bSoundEnabled));
	Root.emplace("fps", model::JsonValue(InProfile.FpsLimit));
	Root.emplace("language", model::JsonValue(InProfile.CurrentLanguage));
	Root.emplace("vsync", model::JsonValue(InProfile.bVerticalSync));
	Root.emplace("window_mode", model::JsonValue(static_cast<int>(InProfile.CurrentWindowMode)));
	Root.emplace("map_scale", model::JsonValue(InProfile.MapScale));
	Root.emplace("smooth_pixels", model::JsonValue(InProfile.bSmoothPixels));
	Root.emplace("hd_textures", model::JsonValue(InProfile.bHdTextures));
	if ( InProfile.WindowPosition )
	{
		Root.emplace("window_x", model::JsonValue(InProfile.WindowPosition->X));
		Root.emplace("window_y", model::JsonValue(InProfile.WindowPosition->Y));
	}
	Root.emplace("effects_volume", model::JsonValue(InProfile.EffectsVolume));
	Root.emplace("music_volume", model::JsonValue(InProfile.MusicVolume));
	Root.emplace("audio_device", model::JsonValue(InProfile.AudioDevice));
	Root.emplace("text_size", model::JsonValue(static_cast<int>(InProfile.CurrentTextSize)));

	model::JsonValue::Object Upgrades;
	for ( const auto& Upgrade : InProfile.Upgrades )
	{
		Upgrades.emplace(Upgrade.first, model::JsonValue(Upgrade.second));
	}
	Root.emplace("upgrades", model::JsonValue(std::move(Upgrades)));
	model::JsonValue::Object BestScores;
	for ( const auto& Best : InProfile.BestScores )
	{
		BestScores.emplace(Best.first, model::JsonValue(Best.second));
	}
	Root.emplace("best_scores", model::JsonValue(std::move(BestScores)));

	model::JsonValue::Object Counters;
	for ( const auto& Counter : InProfile.Counters )
	{
		Counters.emplace(Counter.first, model::JsonValue(Counter.second));
	}
	Root.emplace("counters", model::JsonValue(std::move(Counters)));

	model::JsonValue::Object Achievements;
	for ( const std::string& Id : InProfile.Achievements )
	{
		Achievements.emplace(Id, model::JsonValue(true));
	}
	Root.emplace("achievements", model::JsonValue(std::move(Achievements)));
	Root.emplace("difficulty", model::JsonValue(InProfile.Difficulty));

	model::JsonValue::Object LevelDeaths;
	for ( const auto& Deaths : InProfile.LevelDeaths )
	{
		LevelDeaths.emplace(std::to_string(Deaths.first), model::JsonValue(Deaths.second));
	}
	Root.emplace("level_deaths", model::JsonValue(std::move(LevelDeaths)));
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
	Profile.CurrentWindowMode = InNode.GetValueOr("window_mode", 0) == static_cast<int>(WindowMode::Borderless) ? WindowMode::Borderless : WindowMode::Windowed;
	Profile.MapScale = InNode.GetValueOr("map_scale", Profile.MapScale);
	Profile.bSmoothPixels = InNode.GetValueOr("smooth_pixels", Profile.bSmoothPixels);
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
			Profile.Achievements.insert(Id);
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

} // namespace tanks

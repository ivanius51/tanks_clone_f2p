#include "services/AudioManager.h"

#include <algorithm>
#include <array>

#include "audio/include/SimpleAudioEngine.h"

namespace tanks
{

namespace
{
	const char* SoundFile(const Sound InSound)
	{
		switch ( InSound )
		{
			case Sound::PlayerShot: return "sounds/player_shot.wav";
			case Sound::HitWall: return "sounds/hit_wall.wav";
			case Sound::HitArmor: return "sounds/hit_armor.wav";
			case Sound::EnemyExplode: return "sounds/enemy_explode.wav";
			case Sound::PlayerExplode: return "sounds/player_explode.wav";
			case Sound::BonusPickup: return "sounds/bonus_pickup.wav";
			case Sound::GameOver: return "sounds/game_over.wav";
			case Sound::LevelComplete: return "sounds/level_complete.wav";
			case Sound::Pause: return "sounds/pause.wav";
			case Sound::LevelStart: return "sounds/level_start.wav";
			case Sound::WallBreak: return "sounds/wall_break.wav";
			case Sound::BossExplode: return "sounds/boss_explode.wav";
		}
		return nullptr;
	}

	const std::array<Sound, 12> AllSounds = {
		Sound::PlayerShot, Sound::HitWall, Sound::HitArmor, Sound::EnemyExplode, Sound::PlayerExplode,
		Sound::BonusPickup, Sound::GameOver, Sound::LevelComplete, Sound::Pause,
		Sound::LevelStart, Sound::WallBreak, Sound::BossExplode
	};
}

void AudioManager::Preload()
{
	auto Engine = CocosDenshion::SimpleAudioEngine::getInstance();
	for ( Sound CurrentSound : AllSounds )
	{
		Engine->preloadEffect(SoundFile(CurrentSound));
	}
}

void AudioManager::Play(const Sound InSound)
{
	if ( !bEnabled )
	{
		return;
	}
	if ( const char* File = SoundFile(InSound) )
	{
		CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(File);
	}
}

void AudioManager::SetEnabled(const bool bInEnabled)
{
	bEnabled = bInEnabled;
	if ( !bEnabled )
	{
		CocosDenshion::SimpleAudioEngine::getInstance()->stopAllEffects();
	}
}

bool AudioManager::IsEnabled() const
{
	return bEnabled;
}

void AudioManager::SetVolume(const float InVolume)
{
	Volume = std::max(0.0f, std::min(1.0f, InVolume));
	CocosDenshion::SimpleAudioEngine::getInstance()->setEffectsVolume(Volume);
}

float AudioManager::GetVolume() const
{
	return Volume;
}

AudioManager::~AudioManager()
{
	CocosDenshion::SimpleAudioEngine::end();
}

} // namespace tanks

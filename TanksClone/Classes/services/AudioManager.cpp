#include "services/AudioManager.h"

#include <algorithm>
#include <array>
#include <string>

#include "audio/include/SimpleAudioEngine.h"

#include "core/GameContext.h"
#include "services/ResourceManager.h"

namespace tanks
{

namespace
{
	std::string SoundPath(const Sound InSound)
	{
		return GameContext::Get().GetManager<ResourceManager>().ResolvePath(AudioManager::GetSoundId(InSound), model::ResourceKind::Sound);
	}

	const std::array<Sound, 12> AllSounds = {
		Sound::PlayerShot, Sound::HitWall, Sound::HitArmor, Sound::EnemyExplode, Sound::PlayerExplode,
		Sound::BonusPickup, Sound::GameOver, Sound::LevelComplete, Sound::Pause,
		Sound::LevelStart, Sound::WallBreak, Sound::BossExplode
	};
}

const char* AudioManager::GetSoundId(const Sound InSound)
{
	switch ( InSound )
	{
		case Sound::PlayerShot: return "sfx.player_shot";
		case Sound::HitWall: return "sfx.hit_wall";
		case Sound::HitArmor: return "sfx.hit_armor";
		case Sound::EnemyExplode: return "sfx.enemy_explode";
		case Sound::PlayerExplode: return "sfx.player_explode";
		case Sound::BonusPickup: return "sfx.bonus_pickup";
		case Sound::GameOver: return "sfx.game_over";
		case Sound::LevelComplete: return "sfx.level_complete";
		case Sound::Pause: return "sfx.pause";
		case Sound::LevelStart: return "sfx.level_start";
		case Sound::WallBreak: return "sfx.wall_break";
		case Sound::BossExplode: return "sfx.boss_explode";
	}
	return nullptr;
}

void AudioManager::Preload()
{
	auto Engine = CocosDenshion::SimpleAudioEngine::getInstance();
	for ( Sound CurrentSound : AllSounds )
	{
		const std::string Path = SoundPath(CurrentSound);
		if ( !Path.empty() )
		{
			Engine->preloadEffect(Path.c_str());
		}
	}
}

void AudioManager::Play(const Sound InSound)
{
	if ( !bEnabled )
	{
		return;
	}
	const std::string Path = SoundPath(InSound);
	if ( !Path.empty() )
	{
		CocosDenshion::SimpleAudioEngine::getInstance()->playEffect(Path.c_str());
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

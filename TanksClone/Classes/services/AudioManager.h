#pragma once

namespace tanks
{

enum class Sound
{
	PlayerShot,
	HitWall,
	HitArmor,
	EnemyExplode,
	PlayerExplode,
	BonusPickup,
	GameOver,
	LevelComplete,
	Pause,
	LevelStart,
	WallBreak,
	BossExplode
};

// Plays sound effects. Does nothing while sound is disabled.
class AudioManager
{
public:
	AudioManager() = default;
	~AudioManager();
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	// Loads all sounds so that the first shot does not stutter.
	void Preload();
	void Play(Sound InSound);

	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const;
	// Volume of the sound effects, 0..1.
	void SetVolume(float InVolume);
	float GetVolume() const;

private:
	bool bEnabled = true;
	float Volume = 1.0f;
};

} // namespace tanks

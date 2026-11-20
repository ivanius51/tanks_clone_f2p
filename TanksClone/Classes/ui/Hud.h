#pragma once

#include <string>
#include <vector>

#include "cocos2d.h"

#include "core/GameSession.h"

namespace tanks
{
class Ability;
class Tank;
}

namespace tanks::view::cocos
{

// The game's HUD, laid out like a MOBA:
//  - a thin bar at the top with the level: name, wave, enemies left, score and coins;
//  - the player's panel at the bottom: portrait, health and ammo bars, stats, ability
//    slots (hotkey, icon, cooldown) and the active buffs, each with a progress bar under it.
// Put it at the origin of the screen.
class Hud : public cocos2d::Node
{
public:
	// The session must outlive the HUD.
	static Hud* create(const model::GameSession* InSession);

	void update(float InDelta) override;

private:
	// One ability slot of the panel.
	struct Slot
	{
		cocos2d::Node* Root = nullptr;
		cocos2d::Node* Icon = nullptr;
		std::string ShownIcon;
		cocos2d::DrawNode* Cooldown = nullptr;
		cocos2d::Label* Timer = nullptr;
		int ShownCooldown = -1;
		int ShownSeconds = -1;
	};

	bool init(const model::GameSession* InSession);
	void BuildTopBar();
	void BuildPlayerPanel();
	void BuildSlots(Tank& InPlayer);

	void UpdateTopBar();
	void UpdatePanel(Tank* InPlayer);
	void UpdateSlots(Tank* InPlayer);

	cocos2d::Label* AddLabel(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, float InFontSize,
		const cocos2d::Vec2& InAnchor = cocos2d::Vec2(0.0f, 0.5f));
	void DrawBar(cocos2d::DrawNode* InNode, const cocos2d::Rect& InArea, float InFraction, const cocos2d::Color4F& InColor);
	cocos2d::Sprite* CreateIcon(const std::string& InFrame, const cocos2d::Color3B& InColor, float InSize);
	cocos2d::Node* CreateAbilityIcon(const Ability& InAbility, float InSize);

	const model::GameSession* Session = nullptr;

	// Top bar
	cocos2d::Node* Top = nullptr;
	cocos2d::Label* LevelLabel = nullptr;
	cocos2d::Label* WaveLabel = nullptr;
	cocos2d::Label* EnemiesLabel = nullptr;
	cocos2d::DrawNode* EnemiesBar = nullptr;
	cocos2d::Label* ScoreLabel = nullptr;

	// Player panel
	cocos2d::Node* Panel = nullptr;
	cocos2d::DrawNode* HealthBar = nullptr;
	cocos2d::DrawNode* AmmoBar = nullptr;
	cocos2d::DrawNode* ReloadBar = nullptr;
	cocos2d::Label* HealthLabel = nullptr;
	cocos2d::Label* AmmoLabel = nullptr;
	cocos2d::Label* StatsLabel = nullptr;
	cocos2d::Node* SlotsRoot = nullptr;
	std::vector<Slot> Slots;
	bool bSlotsBuilt = false;

	// Last shown values: labels and bars are only rebuilt when something changes.
	std::string ShownHealth;
	std::string ShownAmmo;
	std::string ShownStats;
	std::string ShownEnemies;
	std::string ShownScore;
	std::string ShownWave;
	int ShownHealthPercent = -1;
	int ShownAmmoPercent = -1;
	int ShownReloadPercent = -1;
	int ShownEnemiesPercent = -1;
};

} // namespace tanks::view::cocos

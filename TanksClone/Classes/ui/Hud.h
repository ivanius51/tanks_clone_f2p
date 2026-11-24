#pragma once

#include <string>
#include <vector>

#include "cocos2d.h"

#include "core/GameSession.h"
#include "model/InputTypes.h"
#include "model/ScreenLayout.h"

namespace tanks
{
class Ability;
class Tank;
}

namespace tanks::view::cocos
{
namespace widgets { class Button; }

class LayoutWidget;

/**
 * The game's HUD, laid out like a MOBA:
 * - a thin bar at the top with the level: name, wave, enemies left, score and coins;
 * - the player's panel at the bottom: portrait, health and ammo bars, stats, ability
 * slots (hotkey, icon, cooldown) and the active buffs, each with a progress bar under it.
 * The bars float: they stretch over the whole window width and stick to its top and bottom edges; the level
 * texts and the player's panel to the left corners, the score to the right, the ability slots to the middle.
 * Put it at the origin of the design area.
 */
class Hud : public cocos2d::Node
{
public:
	static Hud* create(const model::GameSession* InSession);

	void update(float InDelta) override;

private:
	/** One ability slot of the panel. */
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
	widgets::Button* CreateSwitchButton(const std::string& InKeyText);
	void Relayout(const model::ScreenLayout& InLayout);
	void DrawBand(cocos2d::DrawNode* InNode, float InWidth, float InHeight);

	void UpdateTopBar();
	void UpdatePanel(Tank* InPlayer);
	void UpdateSlots(Tank* InPlayer);

	cocos2d::Label* AddLabel(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, float InFontSize,
		const cocos2d::Vec2& InAnchor = cocos2d::Vec2(0.0f, 0.5f));
	cocos2d::Sprite* CreateIcon(const std::string& InFrame, const cocos2d::Color3B& InColor, float InSize);
	cocos2d::Node* CreateAbilityIcon(const Ability& InAbility, float InSize);

	const model::GameSession* Session = nullptr;

	cocos2d::Node* Top = nullptr;
	cocos2d::DrawNode* TopBackground = nullptr;
	cocos2d::Label* LevelLabel = nullptr;
	cocos2d::Label* WaveLabel = nullptr;
	cocos2d::Label* EnemiesLabel = nullptr;
	LayoutWidget* EnemiesBar = nullptr;
	cocos2d::Label* ScoreLabel = nullptr;

	cocos2d::Node* Panel = nullptr;
	cocos2d::DrawNode* PanelBackground = nullptr;
	LayoutWidget* HealthBar = nullptr;
	LayoutWidget* AmmoBar = nullptr;
	LayoutWidget* ReloadBar = nullptr;
	cocos2d::Label* StatsLabel = nullptr;
	cocos2d::Node* SlotsRoot = nullptr;
	std::vector<Slot> Slots;
	bool bSlotsBuilt = false;
	model::InputDevice SlotsDevice = model::InputDevice::Keyboard;

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

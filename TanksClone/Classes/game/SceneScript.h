#pragma once

#include <map>
#include <string>

#include "cocos2d.h"

#include "model/ScriptPlayer.h"
#include "templates/TFunction.h"

namespace game
{
class Tank;

/**
 * Runs a level's script over real tanks: enemies are bound to actors by spawn order ("enemy#1"),
 * their AI is paused and friendly fire is on while the script runs, then the AI takes over again.
 */
class SceneScript : public model::IScriptWorld
{
public:
	SceneScript(const model::Script& InScript, int InMapRows, TFunction<void(const std::string&)> InShowCaption);

	void OnEnemySpawned(Tank& InEnemy);
	void Update(float InDelta);
	bool IsRunning() const;

	bool HasActor(const std::string& InActor) const override;
	bool IsGone(const std::string& InActor) const override;
	model::Vec2f GetCell(const std::string& InActor) const override;
	bool IsAt(const std::string& InActor, const model::Vec2f& InCell) const override;
	bool IsFacing(const std::string& InActor, const std::string& InTarget) const override;
	bool HasEvent(const std::string& InEvent) const override;
	void MoveTo(const std::string& InActor, const model::Vec2f& InCell) override;
	void Face(const std::string& InActor, const std::string& InTarget) override;
	void Shoot(const std::string& InActor) override;
	void ShowCaption(const std::string& InText) override;
	void StopActor(const std::string& InActor) override;
	void SetHealth(const std::string& InActor, float InShare) override;
	void ApplyBuff(const std::string& InActor, const std::string& InBuff) override;
	void SetTactic(const std::string& InActor, const std::string& InTactic) override;
	void EndLevel(bool bInWin) override;
	int GetCounter(const std::string& InName) const override;
	bool IsPlayerInArea(const model::Vec2f& InOrigin, const model::Vec2f& InSize) const override;

	void SetCounterSource(TFunction<int(const std::string&)> InSource) { CounterSource = std::move(InSource); }
	void SetPlayerSource(TFunction<const Tank*()> InSource) { PlayerSource = std::move(InSource); }
	void SetEndLevelHandler(TFunction<void(bool)> InHandler) { EndLevelHandler = std::move(InHandler); }

private:
	Tank* FindActor(const std::string& InActor) const;
	void SteerTowards(Tank& InActor, const cocos2d::Vec2& InPoint, bool bInMove);
	void ReleaseActors();

	float CellToWorldY(float InRow) const;
	float WorldYToRow(float InWorldY) const;

	int MapRows;
	model::ScriptPlayer Player;
	TFunction<void(const std::string&)> ShowCaptionText;
	TFunction<void(bool)> EndLevelHandler;
	TFunction<int(const std::string&)> CounterSource;
	TFunction<const Tank*()> PlayerSource;
	std::map<std::string, std::string> SlotToActor;
	std::map<std::string, Tank*> Actors;
	std::map<std::string, bool> Bound;
	int EnemiesSpawned = 0;
	bool bReleased = false;
};
} // namespace game

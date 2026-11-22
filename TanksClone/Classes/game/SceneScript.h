#pragma once

#include <functional>
#include <map>
#include <string>

#include "cocos2d.h"

#include "model/ScriptPlayer.h"

namespace tanks
{

class Tank;

/**
 * Runs a level's script over real tanks: enemies are bound to actors by spawn order ("enemy#1"),
 * their AI is paused and friendly fire is on while the script runs, then the AI takes over again.
 */
class SceneScript : public model::IScriptWorld
{
public:
	SceneScript(const model::Script& InScript, std::function<void(const std::string&)> InShowCaption);

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

private:
	Tank* FindActor(const std::string& InActor) const;
	void SteerTowards(Tank& InActor, const cocos2d::Vec2& InPoint, bool bInMove);
	void ReleaseActors();

	model::ScriptPlayer Player;
	std::function<void(const std::string&)> ShowCaptionText;
	std::map<std::string, std::string> SlotToActor; // "enemy#1" -> "a"
	std::map<std::string, Tank*> Actors;
	std::map<std::string, bool> Bound;              // an actor that was spawned; missing from Actors once destroyed
	int EnemiesSpawned = 0;
	bool bReleased = false;
};

} // namespace tanks

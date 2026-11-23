#pragma once

#include <map>
#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"
#include "model/MathTypes.h"

namespace tanks::model
{

enum class ScriptStepKind
{
	Wait,
	MoveTo,
	Face,
	Shoot,
	Caption,
	Until,
	SetHealth,
	Buff,
	Tactic,
	EndLevel
};

/** One line of a level "script": what one actor does, or what the scene waits for. */
struct ScriptStep
{
	ScriptStepKind Kind = ScriptStepKind::Wait;
	std::string Actor;
	float Seconds = 0.0f;      // wait time, or the timeout of move_to / face / until
	Vec2f Cell;                // move_to target, in level cells; with move_by, an offset from where the actor stands
	bool bRelative = false;
	std::string LineUpWith;    // move into line with this actor, Gap cells away on the side it comes from
	float Gap = 4.0f;
	std::string Target;        // face: another actor's name
	int Shots = 1;
	float ShotInterval = 0.5f;
	std::string Text;          // caption
	std::string Event;         // until: "b.destroyed" etc.
	float Amount = 1.0f;       // set_health: the share of the maximum health
	bool bWin = true;          // end_level: the level is won (false = lost)
};

/** A level's "script" block: actor names bound to spawn slots ("enemy#1"), then steps run in order. */
struct Script
{
	std::map<std::string, std::string> Actors;
	std::vector<ScriptStep> Steps;

	bool IsEmpty() const;
	static Script FromConfig(const IConfigNode& InNode);
	// The block as written to a level file; reads back as the same script.
	JsonValue ToJson() const;
};

/** What a script acts on; the game implements it over real tanks, tests over a fake. */
class IScriptWorld
{
public:
	virtual ~IScriptWorld() = default;
	virtual bool HasActor(const std::string& InActor) const = 0;
	// An actor that was on the scene and is gone (destroyed); one not spawned yet is not gone.
	virtual bool IsGone(const std::string& InActor) const = 0;
	virtual Vec2f GetCell(const std::string& InActor) const = 0;
	virtual bool IsAt(const std::string& InActor, const Vec2f& InCell) const = 0;
	virtual bool IsFacing(const std::string& InActor, const std::string& InTarget) const = 0;
	virtual bool HasEvent(const std::string& InEvent) const = 0;
	virtual void MoveTo(const std::string& InActor, const Vec2f& InCell) = 0;
	virtual void Face(const std::string& InActor, const std::string& InTarget) = 0;
	virtual void Shoot(const std::string& InActor) = 0;
	virtual void ShowCaption(const std::string& InText) = 0;
	virtual void StopActor(const std::string& InActor) = 0;
	// Setup steps for test and demo scenes; a world that does not support them ignores them.
	virtual void SetHealth(const std::string& InActor, float InShare) { (void)InActor; (void)InShare; }
	virtual void ApplyBuff(const std::string& InActor, const std::string& InBuff) { (void)InActor; (void)InBuff; }
	virtual void SetTactic(const std::string& InActor, const std::string& InTactic) { (void)InActor; (void)InTactic; }
	virtual void EndLevel(bool bInWin) { (void)bInWin; }
	// Numbers a script can wait on in an "until" like "kills>=3": kills, defeated, pickups, wave.
	virtual int GetCounter(const std::string& InName) const { (void)InName; return 0; }
	// For "until": "player.in_area:x,y,w,h" (level cells).
	virtual bool IsPlayerInArea(const Vec2f& InOrigin, const Vec2f& InSize) const { (void)InOrigin; (void)InSize; return false; }
};

/** Runs a Script one step at a time. A step waits for its actor to spawn (up to ActorWaitTime) and is skipped once the actor is gone; move/face/until give up after their timeout. */
class ScriptPlayer
{
public:
	ScriptPlayer() = default;
	explicit ScriptPlayer(Script InScript);

	void Update(float InDelta, IScriptWorld& InWorld);
	bool IsRunning() const;
	std::size_t GetStepIndex() const;
	const Script& GetScript() const;

private:
	bool RunStep(const ScriptStep& InStep, float InDelta, IScriptWorld& InWorld);
	void NextStep();

	Script Current;
	std::size_t StepIndex = 0;
	float StepTime = 0.0f;
	float ShotTimer = 0.0f;
	int ShotsFired = 0;
	bool bMoveTargetSet = false;
	Vec2f MoveTarget;
};

} // namespace tanks::model

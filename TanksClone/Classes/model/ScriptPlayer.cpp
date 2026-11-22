#include "model/ScriptPlayer.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tanks::model
{

namespace
{
	constexpr float DefaultMoveTimeout = 6.0f;
	constexpr float DefaultFaceTimeout = 3.0f;
	constexpr float DefaultUntilTimeout = 5.0f;
	constexpr float ActorWaitTime = 15.0f;
	constexpr float SameLineCells = 2.0f;

	ScriptStep ReadStep(const IConfigNode& InNode)
	{
		ScriptStep Step;
		Step.Actor = InNode.GetValueOr("actor", std::string());
		if ( InNode.Has("move_to") )
		{
			Step.Kind = ScriptStepKind::MoveTo;
			Step.Cell = InNode.GetValueOr("move_to", Vec2f());
			Step.Seconds = InNode.GetValueOr("timeout", DefaultMoveTimeout);
		}
		else if ( InNode.Has("move_by") )
		{
			Step.Kind = ScriptStepKind::MoveTo;
			Step.bRelative = true;
			Step.Cell = InNode.GetValueOr("move_by", Vec2f());
			Step.Seconds = InNode.GetValueOr("timeout", DefaultMoveTimeout);
		}
		else if ( InNode.Has("line_up") )
		{
			Step.Kind = ScriptStepKind::MoveTo;
			Step.LineUpWith = InNode.GetValueOr("line_up", std::string());
			Step.Gap = InNode.GetValueOr("gap", Step.Gap);
			Step.Seconds = InNode.GetValueOr("timeout", DefaultMoveTimeout);
		}
		else if ( InNode.Has("face") )
		{
			Step.Kind = ScriptStepKind::Face;
			Step.Target = InNode.GetValueOr("face", std::string());
			Step.Seconds = InNode.GetValueOr("timeout", DefaultFaceTimeout);
		}
		else if ( InNode.Has("shoot") )
		{
			Step.Kind = ScriptStepKind::Shoot;
			Step.Shots = std::max(1, InNode.GetValueOr("shoot", 1));
			Step.ShotInterval = InNode.GetValueOr("interval", Step.ShotInterval);
		}
		else if ( InNode.Has("caption") )
		{
			Step.Kind = ScriptStepKind::Caption;
			Step.Text = InNode.GetValueOr("caption", std::string());
			Step.Seconds = InNode.GetValueOr("seconds", 0.0f);
		}
		else if ( InNode.Has("until") )
		{
			Step.Kind = ScriptStepKind::Until;
			Step.Event = InNode.GetValueOr("until", std::string());
			Step.Seconds = InNode.GetValueOr("timeout", DefaultUntilTimeout);
		}
		else
		{
			Step.Kind = ScriptStepKind::Wait;
			Step.Seconds = InNode.GetValueOr("wait", 0.0f);
		}
		return Step;
	}
}

bool Script::IsEmpty() const
{
	return Steps.empty();
}

Script Script::FromConfig(const IConfigNode& InNode)
{
	Script Result;
	Result.Actors = InNode.GetValueOr("actors", Result.Actors);
	if ( const IConfigNode* const Steps = InNode.FindChild("steps") )
	{
		for ( std::size_t Index = 0; Index < Steps->GetItemCount(); ++Index )
		{
			if ( const IConfigNode* const Item = Steps->GetItem(Index) )
			{
				Result.Steps.push_back(ReadStep(*Item));
			}
		}
	}
	return Result;
}

ScriptPlayer::ScriptPlayer(Script InScript)
	: Current(std::move(InScript))
{
}

bool ScriptPlayer::IsRunning() const
{
	return StepIndex < Current.Steps.size();
}

std::size_t ScriptPlayer::GetStepIndex() const
{
	return StepIndex;
}

const Script& ScriptPlayer::GetScript() const
{
	return Current;
}

void ScriptPlayer::NextStep()
{
	++StepIndex;
	StepTime = 0.0f;
	ShotTimer = 0.0f;
	ShotsFired = 0;
	bMoveTargetSet = false;
}

void ScriptPlayer::Update(const float InDelta, IScriptWorld& InWorld)
{
	// Instant steps (caption, a finished move) chain within one frame; a step that waits stops the loop.
	float Delta = InDelta;
	while ( IsRunning() )
	{
		if ( !RunStep(Current.Steps[StepIndex], Delta, InWorld) )
		{
			return;
		}
		NextStep();
		Delta = 0.0f;
	}
}

bool ScriptPlayer::RunStep(const ScriptStep& InStep, const float InDelta, IScriptWorld& InWorld)
{
	StepTime += InDelta;
	const bool bNeedsActor = InStep.Kind == ScriptStepKind::MoveTo || InStep.Kind == ScriptStepKind::Face || InStep.Kind == ScriptStepKind::Shoot;
	if ( bNeedsActor && !InWorld.HasActor(InStep.Actor) )
	{
		const bool bGaveUp = InWorld.IsGone(InStep.Actor) || StepTime >= ActorWaitTime;
		StepTime = bGaveUp ? StepTime : 0.0f;
		return bGaveUp;
	}
	switch ( InStep.Kind )
	{
	case ScriptStepKind::Wait:
		return StepTime >= InStep.Seconds;
	case ScriptStepKind::MoveTo:
		if ( !bMoveTargetSet )
		{
			const Vec2f From = InWorld.GetCell(InStep.Actor);
			MoveTarget = InStep.bRelative ? Vec2f(From.X + InStep.Cell.X, From.Y + InStep.Cell.Y) : InStep.Cell;
			if ( !InStep.LineUpWith.empty() )
			{
				const Vec2f Mate = InWorld.GetCell(InStep.LineUpWith);
				const bool bSameColumn = std::abs(From.X - Mate.X) < SameLineCells;
				MoveTarget = bSameColumn ? Vec2f(Mate.X, Mate.Y + (From.Y < Mate.Y ? -InStep.Gap : InStep.Gap))
					: Vec2f(Mate.X + (From.X < Mate.X ? -InStep.Gap : InStep.Gap), Mate.Y);
			}
			bMoveTargetSet = true;
		}
		if ( InWorld.IsAt(InStep.Actor, MoveTarget) || StepTime >= InStep.Seconds )
		{
			InWorld.StopActor(InStep.Actor);
			return true;
		}
		InWorld.MoveTo(InStep.Actor, MoveTarget);
		return false;
	case ScriptStepKind::Face:
		if ( InWorld.IsFacing(InStep.Actor, InStep.Target) || StepTime >= InStep.Seconds || !InWorld.HasActor(InStep.Target) )
		{
			InWorld.StopActor(InStep.Actor);
			return true;
		}
		InWorld.Face(InStep.Actor, InStep.Target);
		return false;
	case ScriptStepKind::Shoot:
		ShotTimer -= InDelta;
		if ( ShotTimer > 0.0f )
		{
			return false;
		}
		InWorld.Shoot(InStep.Actor);
		ShotTimer = InStep.ShotInterval;
		return ++ShotsFired >= InStep.Shots;
	case ScriptStepKind::Caption:
		if ( ShotsFired == 0 )
		{
			InWorld.ShowCaption(InStep.Text);
			ShotsFired = 1;
		}
		return StepTime >= InStep.Seconds;
	case ScriptStepKind::Until:
		return InWorld.HasEvent(InStep.Event) || StepTime >= InStep.Seconds;
	}
	return true;
}

} // namespace tanks::model

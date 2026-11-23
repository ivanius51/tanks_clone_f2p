#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::model
{

enum class Easing
{
	Linear,
	In,
	Out,
	InOut,
	BackIn,
	BackOut,
	BackInOut,
	SineIn,
	SineOut,
	SineInOut,
	BounceIn,
	BounceOut,
	BounceInOut,
	ElasticOut
};

enum class AnimProperty
{
	Alpha,
	Scale,
	ScaleX,
	ScaleY,
	Rotation,
	MoveX,
	MoveY
};

bool EnumFromName(std::string_view InName, Easing& OutEasing);
bool EnumFromName(std::string_view InName, AnimProperty& OutProperty);

// InTime 0..1 in, the eased share 0..1 out (back and elastic curves leave the range on the way).
float ApplyEasing(Easing InEasing, float InTime);

/** One change of one property over time. */
struct AnimStep
{
	AnimProperty Property = AnimProperty::Alpha;
	// Absent: the value the property had before this step (the identity when no earlier step touched it).
	float From = 0.0f;
	bool bHasFrom = false;
	float To = 0.0f;
	float Delay = 0.0f;
	float Duration = 0.0f;
	Easing Ease = Easing::Linear;

	bool FromConfig(const IConfigNode& InNode);
};

/** An animation file (configs/ui/animations/<name>.json): steps that run together, or one after another. */
struct AnimationDef
{
	std::string Id;
	std::vector<AnimStep> Steps;
	// Each step starts when the previous one (with its delay) has ended; otherwise all start together after their delay.
	bool bSequence = false;
	// Starts again from the identity values when it ends.
	bool bLoop = false;
	// A loop plays forwards and backwards (needs bLoop).
	bool bPingPong = false;

	static AnimationDef FromConfig(const IConfigNode& InNode, const std::string& InId, std::vector<std::string>& OutMissing);
};

/** What an animation asks of a node at one moment, as changes on top of its normal look. */
struct AnimationValues
{
	float Alpha = 1.0f;
	float ScaleX = 1.0f;
	float ScaleY = 1.0f;
	float Rotation = 0.0f;
	float MoveX = 0.0f;
	float MoveY = 0.0f;
};

/** Plays an AnimationDef: Advance with the frame time, Sample for the values. No engine types. */
class AnimationPlayer
{
public:
	AnimationPlayer() = default;
	explicit AnimationPlayer(const AnimationDef& InDef);

	void Advance(float InDelta);
	void Restart();
	// A loop never finishes.
	bool IsFinished() const;
	float GetTime() const;
	float GetTotalTime() const;
	AnimationValues Sample() const;

private:
	struct Resolved
	{
		AnimStep Step;
		float Start = 0.0f;
	};

	static void Apply(AnimationValues& OutValues, AnimProperty InProperty, float InValue);
	float LocalTime() const;

	std::vector<Resolved> Steps;
	float Time = 0.0f;
	float Total = 0.0f;
	bool bLoop = false;
	bool bPingPong = false;
};

} // namespace tanks::model

#include "model/Animation.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace tanks::model
{

namespace
{
	constexpr float Pi = 3.14159265358979f;
	constexpr float BackOvershoot = 1.70158f;

	float BounceOut(float InTime)
	{
		constexpr float N = 7.5625f;
		constexpr float D = 2.75f;
		if ( InTime < 1.0f / D )
		{
			return N * InTime * InTime;
		}
		if ( InTime < 2.0f / D )
		{
			InTime -= 1.5f / D;
			return N * InTime * InTime + 0.75f;
		}
		if ( InTime < 2.5f / D )
		{
			InTime -= 2.25f / D;
			return N * InTime * InTime + 0.9375f;
		}
		InTime -= 2.625f / D;
		return N * InTime * InTime + 0.984375f;
	}

	float Identity(const AnimProperty InProperty)
	{
		return InProperty == AnimProperty::Alpha || InProperty == AnimProperty::Scale || InProperty == AnimProperty::ScaleX || InProperty == AnimProperty::ScaleY ? 1.0f : 0.0f;
	}
}

bool EnumFromName(const std::string_view InName, Easing& OutEasing)
{
	struct Named
	{
		std::string_view Name;
		Easing Value;
	};
	static constexpr std::array<Named, 14> Names{ {
		{ "linear", Easing::Linear }, { "in", Easing::In }, { "out", Easing::Out }, { "in_out", Easing::InOut },
		{ "back_in", Easing::BackIn }, { "back_out", Easing::BackOut }, { "back_in_out", Easing::BackInOut },
		{ "sine_in", Easing::SineIn }, { "sine_out", Easing::SineOut }, { "sine_in_out", Easing::SineInOut },
		{ "bounce_in", Easing::BounceIn }, { "bounce_out", Easing::BounceOut }, { "bounce_in_out", Easing::BounceInOut },
		{ "elastic_out", Easing::ElasticOut } } };
	for ( const Named& Current : Names )
	{
		if ( Current.Name == InName )
		{
			OutEasing = Current.Value;
			return true;
		}
	}
	return false;
}

bool EnumFromName(const std::string_view InName, AnimProperty& OutProperty)
{
	if ( InName == "alpha" ) { OutProperty = AnimProperty::Alpha; return true; }
	if ( InName == "scale" ) { OutProperty = AnimProperty::Scale; return true; }
	if ( InName == "scale_x" ) { OutProperty = AnimProperty::ScaleX; return true; }
	if ( InName == "scale_y" ) { OutProperty = AnimProperty::ScaleY; return true; }
	if ( InName == "rotation" ) { OutProperty = AnimProperty::Rotation; return true; }
	if ( InName == "move_x" ) { OutProperty = AnimProperty::MoveX; return true; }
	if ( InName == "move_y" ) { OutProperty = AnimProperty::MoveY; return true; }
	return false;
}

std::string_view NameOf(const Easing InEasing)
{
	static constexpr std::array<std::string_view, 14> Names{ "linear", "in", "out", "in_out", "back_in", "back_out", "back_in_out",
		"sine_in", "sine_out", "sine_in_out", "bounce_in", "bounce_out", "bounce_in_out", "elastic_out" };
	static_assert(static_cast<size_t>(Easing::ElasticOut) + 1 == 14, "the names follow the enum order");
	return Names[static_cast<size_t>(InEasing)];
}

float ApplyEasing(const Easing InEasing, const float InTime)
{
	const float T = std::clamp(InTime, 0.0f, 1.0f);
	constexpr float InOutOvershoot = BackOvershoot * 1.525f;
	switch ( InEasing )
	{
		case Easing::In: return T * T;
		case Easing::Out: return 1.0f - (1.0f - T) * (1.0f - T);
		case Easing::InOut: return T < 0.5f ? 2.0f * T * T : 1.0f - 2.0f * (1.0f - T) * (1.0f - T);
		case Easing::BackIn: return (BackOvershoot + 1.0f) * T * T * T - BackOvershoot * T * T;
		case Easing::BackOut:
		{
			const float U = T - 1.0f;
			return 1.0f + (BackOvershoot + 1.0f) * U * U * U + BackOvershoot * U * U;
		}
		case Easing::BackInOut:
		{
			if ( T < 0.5f )
			{
				const float U = 2.0f * T;
				return (U * U * ((InOutOvershoot + 1.0f) * U - InOutOvershoot)) / 2.0f;
			}
			const float U = 2.0f * T - 2.0f;
			return (U * U * ((InOutOvershoot + 1.0f) * U + InOutOvershoot) + 2.0f) / 2.0f;
		}
		case Easing::SineIn: return 1.0f - std::cos(T * Pi / 2.0f);
		case Easing::SineOut: return std::sin(T * Pi / 2.0f);
		case Easing::SineInOut: return -(std::cos(Pi * T) - 1.0f) / 2.0f;
		case Easing::BounceIn: return 1.0f - BounceOut(1.0f - T);
		case Easing::BounceOut: return BounceOut(T);
		case Easing::BounceInOut: return T < 0.5f ? (1.0f - BounceOut(1.0f - 2.0f * T)) / 2.0f : (1.0f + BounceOut(2.0f * T - 1.0f)) / 2.0f;
		case Easing::ElasticOut:
			if ( T <= 0.0f || T >= 1.0f )
			{
				return T;
			}
			return std::pow(2.0f, -10.0f * T) * std::sin((T * 10.0f - 0.75f) * (2.0f * Pi / 3.0f)) + 1.0f;
		default: return T;
	}
}

bool AnimStep::FromConfig(const IConfigNode& InNode)
{
	if ( !InNode.TryRead("property", Property) )
	{
		return false;
	}
	bHasFrom = InNode.TryRead("from", From);
	InNode.TryRead("to", To);
	InNode.TryRead("delay", Delay);
	InNode.TryRead("duration", Duration);
	InNode.TryRead("easing", Ease);
	Delay = std::max(Delay, 0.0f);
	Duration = std::max(Duration, 0.0f);
	return true;
}

AnimationDef AnimationDef::FromConfig(const IConfigNode& InNode, const std::string& InId, std::vector<std::string>& OutMissing)
{
	AnimationDef Result;
	Result.Id = InId;
	InNode.TryRead("sequence", Result.bSequence);
	InNode.TryRead("loop", Result.bLoop);
	InNode.TryRead("ping_pong", Result.bPingPong);
	const IConfigNode* const Steps = InNode.FindChild("steps");
	for ( std::size_t Index = 0; Steps && Index < Steps->GetItemCount(); ++Index )
	{
		AnimStep Step;
		if ( Step.FromConfig(*Steps->GetItem(Index)) )
		{
			Result.Steps.push_back(Step);
		}
		else
		{
			OutMissing.push_back(InId + ".steps[" + std::to_string(Index) + "].property");
		}
	}
	return Result;
}

AnimationPlayer::AnimationPlayer(const AnimationDef& InDef)
	: bLoop(InDef.bLoop)
	, bPingPong(InDef.bLoop && InDef.bPingPong)
{
	std::array<float, 7> Last{};
	std::array<bool, 7> bTouched{};
	float Cursor = 0.0f;
	for ( const AnimStep& Current : InDef.Steps )
	{
		Resolved Item;
		Item.Step = Current;
		Item.Start = InDef.bSequence ? Cursor + Current.Delay : Current.Delay;
		const std::size_t Slot = static_cast<std::size_t>(Current.Property);
		if ( !Current.bHasFrom )
		{
			Item.Step.From = bTouched[Slot] ? Last[Slot] : Identity(Current.Property);
			Item.Step.bHasFrom = true;
		}
		Last[Slot] = Current.To;
		bTouched[Slot] = true;
		const float End = Item.Start + Current.Duration;
		Cursor = End;
		Total = std::max(Total, End);
		Steps.push_back(Item);
	}
}

void AnimationPlayer::Advance(const float InDelta)
{
	Time += std::max(InDelta, 0.0f);
	if ( !bLoop && Time > Total )
	{
		Time = Total;
	}
}

void AnimationPlayer::Restart()
{
	Time = 0.0f;
}

bool AnimationPlayer::IsFinished() const
{
	return !bLoop && Time >= Total;
}

float AnimationPlayer::GetTime() const
{
	return Time;
}

float AnimationPlayer::GetTotalTime() const
{
	return Total;
}

float AnimationPlayer::LocalTime() const
{
	if ( !bLoop || Total <= 0.0f )
	{
		return Time;
	}
	if ( bPingPong )
	{
		const float Phase = std::fmod(Time, 2.0f * Total);
		return Phase <= Total ? Phase : 2.0f * Total - Phase;
	}
	return std::fmod(Time, Total);
}

void AnimationPlayer::Apply(AnimationValues& OutValues, const AnimProperty InProperty, const float InValue)
{
	switch ( InProperty )
	{
		case AnimProperty::Alpha: OutValues.Alpha = InValue; break;
		case AnimProperty::Scale: OutValues.ScaleX = OutValues.ScaleY = InValue; break;
		case AnimProperty::ScaleX: OutValues.ScaleX = InValue; break;
		case AnimProperty::ScaleY: OutValues.ScaleY = InValue; break;
		case AnimProperty::Rotation: OutValues.Rotation = InValue; break;
		case AnimProperty::MoveX: OutValues.MoveX = InValue; break;
		case AnimProperty::MoveY: OutValues.MoveY = InValue; break;
	}
}

AnimationValues AnimationPlayer::Sample() const
{
	AnimationValues Values;
	const float Now = LocalTime();
	for ( const Resolved& Current : Steps )
	{
		if ( Now < Current.Start )
		{
			continue;
		}
		const float Share = Current.Step.Duration > 0.0f ? (Now - Current.Start) / Current.Step.Duration : 1.0f;
		const float Eased = ApplyEasing(Current.Step.Ease, Share);
		Apply(Values, Current.Step.Property, Current.Step.From + (Current.Step.To - Current.Step.From) * Eased);
	}
	return Values;
}

} // namespace tanks::model

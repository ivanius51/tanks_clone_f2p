#include "game/TankStateScript.h"

#include <algorithm>
#include <cmath>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/ai/TankAi.h"
#include "game/components/Buff.h"
#include "game/objects/Tank.h"
#include "model/AiTactic.h"
#include "services/ConfigRefs.h"

namespace game
{

namespace
{
	constexpr int HitDamage = 1;
	constexpr float MinEffectInterval = 0.05f;
	const char* const EffectKeyPrefix = "tank_state_effect_";

	const model::IConfigNode& Root()
	{
		return GameContext::Get<ConfigReader>().GetConfig(config_refs::ResolveId(ids::configs::TankStates));
	}

	cocos2d::Vec2 ReadVec(const model::IConfigNode& InNode)
	{
		return cocos2d::Vec2(static_cast<float>(InNode.GetItem(0)->ReadNumber().value_or(0.0)), static_cast<float>(InNode.GetItem(1)->ReadNumber().value_or(0.0)));
	}

	model::AiTactic FindTactic(const std::string& InName)
	{
		for ( size_t Index = 0; Index < static_cast<size_t>(model::AiTactic::Count); ++Index )
		{
			if ( model::GetTacticName(static_cast<model::AiTactic>(Index)) == InName )
			{
				return static_cast<model::AiTactic>(Index);
			}
		}
		return model::AiTactic::Wander;
	}

	void StartEffect(Tank& InTank, const TankStateEffect& InEffect, const size_t InIndex)
	{
		cocos2d::Node* const Visual = InTank.GetVisualNode();
		if ( !Visual || !Visual->getParent() )
		{
			return;
		}
		const std::string Key = EffectKeyPrefix + std::to_string(InIndex);
		Visual->schedule([Visual, InEffect](float)
		{
			effects::Play(InEffect.Effect, Visual->getParent(), Visual->getPosition() - InEffect.Shot * (TileSize * 0.5f), InEffect.Shot);
		}, std::max(MinEffectInterval, InEffect.Interval), Key);
	}
}

TankState TankStateScript::FromNode(const model::IConfigNode& InNode)
{
	TankState Result;
	FindNamed(InNode.GetValueOr("state", std::string()), Result);
	Result.Label = InNode.GetValueOr("label", Result.Label);
	Result.HealthShare = InNode.GetValueOr("health", Result.HealthShare);
	Result.Buff = InNode.GetValueOr("buff", Result.Buff);
	Result.Tactic = InNode.GetValueOr("tactic", Result.Tactic);
	if ( const model::IConfigNode* const Hits = InNode.FindChild("hits") )
	{
		Result.Hits.Clear();
		for ( size_t Index = 0; Index < Hits->GetItemCount(); ++Index )
		{
			Result.Hits.PushBack(ReadVec(*Hits->GetItem(Index)));
		}
	}
	if ( const model::IConfigNode* const Effects = InNode.FindChild("effects") )
	{
		Result.Effects.Clear();
		for ( size_t Index = 0; Index < Effects->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Node = *Effects->GetItem(Index);
			TankStateEffect Effect;
			Effect.Effect = Node.GetValueOr("effect", std::string());
			Effect.Interval = Node.GetValueOr("interval", Effect.Interval);
			if ( const model::IConfigNode* const Shot = Node.FindChild("shot") )
			{
				Effect.Shot = ReadVec(*Shot);
			}
			Result.Effects.PushBack(Effect);
		}
	}
	return Result;
}

bool TankStateScript::FindNamed(const std::string_view InId, TankState& OutState)
{
	const model::IConfigNode* const States = Root().FindChild("states");
	const model::IConfigNode* const Node = States && !InId.empty() ? States->FindChild(InId) : nullptr;
	if ( !Node )
	{
		return false;
	}
	OutState = FromNode(*Node);
	return true;
}

TankState TankStateScript::ForHealthShare(const float InHealthShare)
{
	TankState Result;
	const model::IConfigNode* const Stages = Root().FindChild("damage_stages");
	for ( size_t Index = 0; Stages && Index < Stages->GetItemCount(); ++Index )
	{
		const model::IConfigNode& Stage = *Stages->GetItem(Index);
		if ( InHealthShare <= Stage.GetValueOr("up_to", 1.0f) )
		{
			FindNamed(Stage.GetValueOr("state", std::string()), Result);
			return Result;
		}
	}
	return Result;
}

void TankStateScript::Apply(Tank& InTank, const TankState& InState)
{
	for ( const cocos2d::Vec2& Direction : InState.Hits )
	{
		InTank.ApplyDamageFrom(HitDamage, ETeam::Player, Direction);
	}
	InTank.SetHealth(std::max(1, static_cast<int>(std::round(InTank.GetMaxHealth() * InState.HealthShare))));
	if ( !InState.Buff.empty() )
	{
		if ( const BuffDef* const Buff = GameContext::Get<BuffCatalog>().Find(InState.Buff) )
		{
			BuffDef Permanent = *Buff;
			Permanent.Duration = -1.0f;
			InTank.GetBuffs().Add(Permanent);
		}
	}
	if ( !InState.Tactic.empty() && InTank.GetAi() )
	{
		InTank.GetAi()->ForceTactic(FindTactic(InState.Tactic));
	}
	for ( size_t Index = 0; Index < InState.Effects.Size(); ++Index )
	{
		StartEffect(InTank, InState.Effects[Index], Index);
	}
}

} // namespace game

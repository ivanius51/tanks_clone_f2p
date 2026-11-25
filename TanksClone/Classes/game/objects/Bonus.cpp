#include "game/objects/Bonus.h"

#include <cmath>

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"

namespace game
{

std::unique_ptr<Bonus> Bonus::Create(cocos2d::Node* const InNode)
{
	std::unique_ptr<Bonus> CurrentBonus(new Bonus());
	if ( !CurrentBonus->Build() )
	{
		return nullptr;
	}
	CurrentBonus->SetVisualNode(InNode);

	// Gentle pulsing so that it stands out.
	const float Scale = InNode->getScale();
	InNode->runAction(cocos2d::RepeatForever::create(cocos2d::Sequence::create(
		cocos2d::ScaleTo::create(0.5f, Scale * 1.15f),
		cocos2d::ScaleTo::create(0.5f, Scale),
		nullptr)));
	return CurrentBonus;
}

bool Bonus::Build()
{
	const bool bResult = PhysicsObject::Build();
	SetType(ObjectType::Bonus);
	SetPassable(true);
	StartUpdating();
	return bResult;
}

void Bonus::Configure(const cocos2d::ValueMap& InData)
{
	PhysicsObject::Configure(InData);

	auto TypeIt = InData.find("bonus_type");
	if ( TypeIt != InData.end() )
	{
		BonusKind = ParseBonusType(TypeIt->second);
	}
	Amount = DataConverter::GetInt(InData, "amount", Amount);
	BuffId = DataConverter::GetString(InData, "buff", "");
	PickupEffect = DataConverter::GetString(InData, "pickup_effect", PickupEffect);
	Lifetime = DataConverter::GetFloat(InData, "lifetime", Lifetime);
	BlinkTime = DataConverter::GetFloat(InData, "blink_time", BlinkTime);
	BlinkPeriod = std::max(0.01f, DataConverter::GetFloat(InData, "blink_period", BlinkPeriod));
}

void Bonus::SetOnCoins(const TFunction<void(int)>& InOnCoins)
{
	OnCoins = InOnCoins;
}

void Bonus::SetOnPicked(const TFunction<void()>& InOnPicked)
{
	OnPicked = InOnPicked;
}

void Bonus::Update(const float InDelta)
{
	if ( !GetVisualNode() || IsDestroying() )
	{
		return;
	}
	if ( Lifetime > 0.0f )
	{
		Age += InDelta;
		if ( Age >= Lifetime )
		{
			Destroy();
			return;
		}
		const bool bBlinking = Lifetime - Age < BlinkTime;
		GetVisualNode()->setVisible(!bBlinking || std::fmod(Age, BlinkPeriod * 2) < BlinkPeriod);
	}

	auto Player = GameContext::Get<ObjectsFactory>().FindObject<Tank>(PlayerName);
	if ( !Player || !IsCollidedTo(Player) )
	{
		return;
	}

	switch ( BonusKind )
	{
		case BonusType::Ammo:
			Player->AddAmmo(Amount);
			break;
		case BonusType::Repair:
			Player->Heal(Amount);
			break;
		case BonusType::Buff:
			if ( const BuffDef* Buff = GameContext::Get<BuffCatalog>().Find(BuffId) )
			{
				Player->GetBuffs().Add(*Buff);
				Player->RefreshStats();
			}
			break;
		case BonusType::Coins:
			if ( OnCoins )
			{
				OnCoins(Amount);
			}
			break;
		default:
			break;
	}
	if ( OnPicked )
	{
		OnPicked();
	}
	GameContext::Get<AudioManager>().Play(ids::sounds::BonusPickup);
	if ( cocos2d::Node* const CurrentVisualNode = GetVisualNode() )
	{
		effects::Play(PickupEffect, CurrentVisualNode->getParent(), CurrentVisualNode->getPosition());
	}
	Destroy();
}

} // namespace game

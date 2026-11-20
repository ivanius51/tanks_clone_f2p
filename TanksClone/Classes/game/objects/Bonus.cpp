#include "game/objects/Bonus.h"

#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"

namespace tanks
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
}

void Bonus::Update(float /*InDelta*/)
{
	if ( !GetVisualNode() )
	{
		return;
	}

	auto Player = GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName);
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
			if ( const BuffDef* Buff = GameContext::Get().GetManager<BuffCatalog>().Find(BuffId) )
			{
				Player->GetBuffs().Add(*Buff);
				Player->RefreshStats();
			}
			break;
		default:
			break;
	}
	GameContext::Get().GetManager<AudioManager>().Play(Sound::BonusPickup);
	Destroy();
}

} // namespace tanks

#include "game/objects/GameObject.h"

#include <algorithm>
#include <string_view>
#include "core/DataConverter.h"

namespace tanks
{

namespace
{
	constexpr const char* UpdateScheduleKey = "GAME_OBJECT_UPDATE";
}

void GameObject::Configure(const cocos2d::ValueMap& InData)
{
	auto HealthIt = InData.find("health");
	if ( HealthIt != InData.end() )
	{
		SetMaxHealth(HealthIt->second.asInt());
	}

	auto DamageIt = InData.find("damage");
	if ( DamageIt != InData.end() )
	{
		SetDamage(DamageIt->second.asInt());
	}

	SetDestroyable(DataConverter::GetBool(InData, "destroyable", IsDestroyable()));

	auto TeamIt = InData.find("team");
	if ( TeamIt != InData.end() )
	{
		SetTeam(ParseTeam(TeamIt->second));
	}
}

int GameObject::GetHealth() const
{
	return Health;
}

int GameObject::GetMaxHealth() const
{
	return MaxHealth;
}

int GameObject::GetDamage() const
{
	return Damage;
}

bool GameObject::IsMovable() const
{
	return bMovable;
}

bool GameObject::IsDestroyable() const
{
	return bDestroyable;
}

bool GameObject::IsPassable() const
{
	return bPassable;
}

bool GameObject::IsFlyable() const
{
	return bFlyable;
}

bool GameObject::IsPlayer() const
{
	return bIsPlayer;
}

void GameObject::SetPlayer(const bool bInIsPlayer)
{
	bIsPlayer = bInIsPlayer;
}

const std::string& GameObject::GetName() const
{
	return Name;
}

void GameObject::SetName(std::string_view InName)
{
	Name = InName;
}

ObjectType GameObject::GetType() const
{
	return ObjectKind;
}

Team GameObject::GetTeam() const
{
	return OwnerTeam;
}

void GameObject::SetTeam(const Team InTeam)
{
	OwnerTeam = InTeam;
}

cocos2d::Node* GameObject::GetVisualNode() const
{
	return VisualNode;
}

void GameObject::SetOnDestroyCallback(const std::function<void(GameObject*)>& InOnDestroyCallback)
{
	OnDestroyCallback = InOnDestroyCallback;
}

void GameObject::SetOnKilledCallback(const std::function<void(GameObject&)>& InOnKilledCallback)
{
	OnKilledCallback = InOnKilledCallback;
}

void GameObject::ApplyDamage(const int InDamage, Team /*InSource*/)
{
	if ( bDestroying || !bDestroyable || InDamage <= 0 )
	{
		return;
	}

	Health = std::max(0, Health - InDamage);
	OnDamaged(InDamage);

	if ( Health == 0 )
	{
		OnKilled();
		if ( OnKilledCallback )
		{
			OnKilledCallback(*this);
		}
		Destroy();
	}
}

void GameObject::Heal(const int InAmount)
{
	if ( !bDestroying && InAmount > 0 )
	{
		Health = std::min(MaxHealth, Health + InAmount);
	}
}

void GameObject::Destroy()
{
	if ( bDestroying )
	{
		return;
	}
	bDestroying = true;

	if ( !VisualNode )
	{
		return;
	}

	// Removing the node deletes this object (see onDestroy), so do it later and
	// keep the node alive until then.
	cocos2d::Node* CurrentVisualNode = VisualNode;
	CurrentVisualNode->retain();
	CurrentVisualNode->setVisible(false);
	cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([CurrentVisualNode]()
	{
		CurrentVisualNode->removeFromParent();
		CurrentVisualNode->release();
	});
}

bool GameObject::IsDestroying() const
{
	return bDestroying;
}

void GameObject::OnCommandReceived(const GameCommand& /*InCommand*/)
{
}

void GameObject::RescaleMaxHealth(const int InMaxHealth)
{
	const int NewMax = std::max(1, InMaxHealth);
	if ( NewMax > MaxHealth )
	{
		Health += NewMax - MaxHealth;
	}
	MaxHealth = NewMax;
	Health = std::min(Health, MaxHealth);
}

void GameObject::SetMaxHealth(const int InHealth)
{
	MaxHealth = std::max(1, InHealth);
	Health = MaxHealth;
}

void GameObject::SetDamage(const int InDamage)
{
	Damage = InDamage;
}

void GameObject::SetMovable(const bool bInIsMovable)
{
	bMovable = bInIsMovable;
}

void GameObject::SetDestroyable(const bool bInIsDestroyable)
{
	bDestroyable = bInIsDestroyable;
}

void GameObject::SetPassable(const bool bInIsPassable)
{
	bPassable = bInIsPassable;
}

void GameObject::SetFlyable(const bool bInIsFlyable)
{
	bFlyable = bInIsFlyable;
}

void GameObject::SetType(const ObjectType InType)
{
	ObjectKind = InType;
}

void GameObject::SetVisualNode(cocos2d::Node* const InVisualNode)
{
	VisualNode = InVisualNode;
	if ( VisualNode )
	{
		// The object dies together with its node leaving the scene.
		VisualNode->setOnExitCallback([this]() { OnDestroy(); });
	}
}

void GameObject::StartUpdating()
{
	cocos2d::Director::getInstance()->getScheduler()->schedule(
		[this](const float InDelta)
		{
			if ( !bDestroying )
			{
				Update(InDelta);
			}
		},
		this, 0.0f, CC_REPEAT_FOREVER, 0.0f, false, UpdateScheduleKey
	);
}

void GameObject::Update(float /*InDelta*/)
{
}

void GameObject::OnDamaged(int /*InDamage*/)
{
}

void GameObject::OnKilled()
{
}

void GameObject::OnDestroy()
{
	// Without this the scheduler would keep calling Update() on a deleted object.
	cocos2d::Director::getInstance()->getScheduler()->unschedule(UpdateScheduleKey, this);

	// The callback usually makes its owner delete this object, so it must not
	// be a member that is still executing when that happens.
	auto CurrentCallback = std::move(OnDestroyCallback);
	OnDestroyCallback = nullptr;
	if ( CurrentCallback )
	{
		CurrentCallback(this);
	}
}

} // namespace tanks

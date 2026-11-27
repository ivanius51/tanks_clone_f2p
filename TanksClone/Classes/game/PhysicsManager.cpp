#include "game/PhysicsManager.h"

#include "core/GameContext.h"
#include "game/ObjectsFactory.h"
#include "game/objects/PhysicsObject.h"

namespace game
{
void PhysicsManager::Listen(const model::BodyId InBody, IBodyListener* const InListener)
{
	Listeners.PushBack({ InBody, InListener });
}

void PhysicsManager::Bind(const model::BodyId InBody, PhysicsObject* const InObject)
{
	while ( Objects.Size() <= InBody.Value )
	{
		Objects.PushBack(nullptr);
	}
	Objects[InBody.Value] = InObject;
}

PhysicsObject* PhysicsManager::FindObject(const model::BodyId InBody) const
{
	return InBody.Value < Objects.Size() ? Objects[InBody.Value] : nullptr;
}

void PhysicsManager::Forget(const model::BodyId InBody)
{
	if ( InBody.Value < Objects.Size() )
	{
		Objects[InBody.Value] = nullptr;
	}
	for ( size_t Index = 0; Index < Listeners.Size(); ++Index )
	{
		if ( Listeners[Index].first == InBody )
		{
			Listeners.RemoveAtSwap(Index);
			return;
		}
	}
}

void PhysicsManager::SyncBodies()
{
	if ( SyncedFrame == Frame )
	{
		return;
	}
	SyncedFrame = Frame;
	GameContext::Get<ObjectsFactory>().ForEachObject<PhysicsObject>([](PhysicsObject& InObject)
	{
		if ( !InObject.IsDestroying() && InObject.GetVisualNode() )
		{
			InObject.SyncBody();
		}
	});
}

void PhysicsManager::Pump()
{
	if ( bPumping )
	{
		bPumpAgain = true;
		return;
	}
	bPumping = true;
	do
	{
		bPumpAgain = false;
		World.DrainEvents(*this);
	}
	while ( bPumpAgain );
	bPumping = false;
}

void PhysicsManager::OnContact(const model::ContactEvent& InEvent)
{
	for ( const std::pair<model::BodyId, IBodyListener*>& Entry : Listeners )
	{
		if ( Entry.first == InEvent.Self )
		{
			Entry.second->OnContact(InEvent);
			return;
		}
	}
}

void PhysicsManager::OnCarved(const model::CarveEvent& InEvent)
{
	for ( const std::pair<model::BodyId, IBodyListener*>& Entry : Listeners )
	{
		if ( Entry.first == InEvent.Body )
		{
			Entry.second->OnCarved(InEvent);
			return;
		}
	}
}
} // namespace game

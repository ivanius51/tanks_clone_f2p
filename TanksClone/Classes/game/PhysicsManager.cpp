#include "game/PhysicsManager.h"

namespace game
{
void PhysicsManager::Listen(const model::BodyId InBody, IBodyListener* const InListener)
{
	Listeners.PushBack({ InBody, InListener });
}

void PhysicsManager::Forget(const model::BodyId InBody)
{
	for ( size_t Index = 0; Index < Listeners.Size(); ++Index )
	{
		if ( Listeners[Index].first == InBody )
		{
			Listeners.RemoveAtSwap(Index);
			return;
		}
	}
}

void PhysicsManager::Pump()
{
	World.DrainEvents(*this);
}

void PhysicsManager::OnContact(const model::ContactEvent&)
{
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

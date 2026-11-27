#include "model/PhysicsEvents.h"

namespace game::model
{
void PhysicsEventQueue::OnContact(const ContactEvent& InEvent)
{
	Order.PushBack(EKind::Contact);
	Contacts.PushBack(InEvent);
}

void PhysicsEventQueue::OnCarved(const CarveEvent& InEvent)
{
	Order.PushBack(EKind::Carve);
	Carves.PushBack(InEvent);
}

void PhysicsEventQueue::Drain(IPhysicsEventSink& InSink)
{
	// A handler may queue and drain more events, so the batch is taken out before it is played.
	TArray<EKind> PlayOrder(std::move(Order));
	TArray<ContactEvent> PlayContacts(std::move(Contacts));
	TArray<CarveEvent> PlayCarves(std::move(Carves));
	Order.Clear();
	Contacts.Clear();
	Carves.Clear();
	size_t NextContact = 0;
	size_t NextCarve = 0;
	for ( const EKind Kind : PlayOrder )
	{
		if ( Kind == EKind::Contact )
		{
			InSink.OnContact(PlayContacts[NextContact++]);
			continue;
		}
		InSink.OnCarved(PlayCarves[NextCarve++]);
	}
}

bool PhysicsEventQueue::IsEmpty() const
{
	return Order.IsEmpty();
}
} // namespace game::model

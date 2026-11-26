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
	size_t NextContact = 0;
	size_t NextCarve = 0;
	for ( const EKind Kind : Order )
	{
		if ( Kind == EKind::Contact )
		{
			InSink.OnContact(Contacts[NextContact++]);
			continue;
		}
		InSink.OnCarved(Carves[NextCarve++]);
	}
	Order.Clear();
	Contacts.Clear();
	Carves.Clear();
}

bool PhysicsEventQueue::IsEmpty() const
{
	return Order.IsEmpty();
}
} // namespace game::model

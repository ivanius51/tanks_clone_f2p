#include "model/PhysicsEvents.h"

namespace game::model
{
void PhysicsEventQueue::OnWallChipped(const WallChipEvent& InEvent)
{
	Order.PushBack(EKind::Chip);
	Chips.PushBack(InEvent);
}

void PhysicsEventQueue::OnBulletHit(const BulletHitEvent& InEvent)
{
	Order.PushBack(EKind::Hit);
	Hits.PushBack(InEvent);
}

void PhysicsEventQueue::Drain(IPhysicsEventSink& InSink)
{
	size_t NextChip = 0;
	size_t NextHit = 0;
	for ( const EKind Kind : Order )
	{
		if ( Kind == EKind::Chip )
		{
			InSink.OnWallChipped(Chips[NextChip++]);
			continue;
		}
		InSink.OnBulletHit(Hits[NextHit++]);
	}
	Order.Clear();
	Chips.Clear();
	Hits.Clear();
}

bool PhysicsEventQueue::IsEmpty() const
{
	return Order.IsEmpty();
}
} // namespace game::model

#include "services/ResourceLoadJob.h"

namespace tanks
{

ResourceLoadJob::ResourceLoadJob(const ResourceSet& InSet) : Set(InSet)
{
}

bool ResourceLoadJob::Step()
{
	if ( IsFinished() )
	{
		return false;
	}
	const bool bLoaded = NextIndex < Set.Atlases.size()
		? Resources.AddAtlas(Set.Atlases[NextIndex])
		: Resources.AddSound(Set.Sounds[NextIndex - Set.Atlases.size()]);
	if ( !bLoaded )
	{
		++FailedCount;
	}
	++NextIndex;
	return !IsFinished();
}

bool ResourceLoadJob::IsFinished() const
{
	return NextIndex >= Set.GetCount();
}

float ResourceLoadJob::GetProgress() const
{
	return Set.GetCount() == 0 ? 1.0f : static_cast<float>(NextIndex) / static_cast<float>(Set.GetCount());
}

std::size_t ResourceLoadJob::GetFailedCount() const
{
	return FailedCount;
}

ResourceHolder ResourceLoadJob::TakeResources()
{
	return std::move(Resources);
}

} // namespace tanks

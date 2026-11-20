#include "services/ResourceLoadJob.h"

#include "core/GameContext.h"

namespace tanks
{

ResourceLoadJob::ResourceLoadJob(const ResourceSet& InSet, const bool bInAsync) : Set(InSet), bAsync(bInAsync)
{
}

bool ResourceLoadJob::Step()
{
	if ( IsFinished() )
	{
		return false;
	}
	if ( bAsync )
	{
		StepAsync();
	}
	else
	{
		StepSync();
	}
	return !IsFinished();
}

void ResourceLoadJob::StepSync()
{
	if ( NextIndex < Set.Atlases.size() )
	{
		if ( !Shared->Resources.AddAtlas(Set.Atlases[NextIndex]) )
		{
			++Shared->FailedCount;
		}
		++Shared->DoneCount;
	}
	else
	{
		if ( !Shared->Resources.AddSound(Set.Sounds[NextIndex - Set.Atlases.size()]) )
		{
			++Shared->FailedCount;
		}
		++Shared->DoneCount;
	}
	++NextIndex;
}

void ResourceLoadJob::StepAsync()
{
	if ( bRequested )
	{
		return;
	}
	bRequested = true;
	ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
	for ( const std::string& CurrentAtlas : Set.Atlases )
	{
		const std::shared_ptr<State> Target = Shared;
		Manager.AcquireAtlasAsync(CurrentAtlas, [Target, CurrentAtlas](AtlasHandle InHandle)
		{
			if ( InHandle )
			{
				Target->Resources.AddAtlas(CurrentAtlas, std::move(InHandle));
			}
			else
			{
				++Target->FailedCount;
			}
			++Target->DoneCount;
		});
	}
	for ( const std::string& CurrentSound : Set.Sounds )
	{
		const std::shared_ptr<State> Target = Shared;
		Manager.AcquireSoundAsync(CurrentSound, [Target, CurrentSound](SoundHandle InHandle)
		{
			if ( InHandle )
			{
				Target->Resources.AddSound(CurrentSound, std::move(InHandle));
			}
			else
			{
				++Target->FailedCount;
			}
			++Target->DoneCount;
		});
	}
}

bool ResourceLoadJob::IsFinished() const
{
	return Shared->DoneCount >= Set.GetCount();
}

float ResourceLoadJob::GetProgress() const
{
	return Set.GetCount() == 0 ? 1.0f : static_cast<float>(Shared->DoneCount) / static_cast<float>(Set.GetCount());
}

std::size_t ResourceLoadJob::GetFailedCount() const
{
	return Shared->FailedCount;
}

ResourceHolder ResourceLoadJob::TakeResources()
{
	return std::move(Shared->Resources);
}

} // namespace tanks

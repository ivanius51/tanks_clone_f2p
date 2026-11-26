#include "services/ResourceLoadJob.h"

#include "core/GameContext.h"

namespace game
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
	if ( NextIndex < Set.Atlases.Size() )
	{
		if ( !Shared->Get().Resources.AddAtlas(Set.Atlases[NextIndex]) )
		{
			++Shared->Get().FailedCount;
		}
		++Shared->Get().DoneCount;
	}
	else
	{
		if ( !Shared->Get().Resources.AddSound(Set.Sounds[NextIndex - Set.Atlases.Size()]) )
		{
			++Shared->Get().FailedCount;
		}
		++Shared->Get().DoneCount;
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
	ResourceManager& Manager = GameContext::Get<ResourceManager>();
	for ( const std::string& CurrentAtlas : Set.Atlases )
	{
		const TSharedHandle<State> Target = Shared;
		Manager.AcquireAtlasAsync(CurrentAtlas, [Target, CurrentAtlas](AtlasHandle InHandle)
		{
			if ( InHandle )
			{
				Target->Get().Resources.AddAtlas(CurrentAtlas, std::move(InHandle));
			}
			else
			{
				++Target->Get().FailedCount;
			}
			++Target->Get().DoneCount;
		});
	}
	for ( const std::string& CurrentSound : Set.Sounds )
	{
		const TSharedHandle<State> Target = Shared;
		Manager.AcquireSoundAsync(CurrentSound, [Target, CurrentSound](SoundHandle InHandle)
		{
			if ( InHandle )
			{
				Target->Get().Resources.AddSound(CurrentSound, std::move(InHandle));
			}
			else
			{
				++Target->Get().FailedCount;
			}
			++Target->Get().DoneCount;
		});
	}
}

bool ResourceLoadJob::IsFinished() const
{
	return Shared->Get().DoneCount >= Set.GetCount();
}

float ResourceLoadJob::GetProgress() const
{
	return Set.GetCount() == 0 ? 1.0f : static_cast<float>(Shared->Get().DoneCount) / static_cast<float>(Set.GetCount());
}

std::size_t ResourceLoadJob::GetFailedCount() const
{
	return Shared->Get().FailedCount;
}

ResourceHolder ResourceLoadJob::TakeResources()
{
	return std::move(Shared->Get().Resources);
}

} // namespace game

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

void ResourceLoadJob::LoadSound(const std::string& InSound)
{
	if ( !Shared->Resources.AddSound(InSound) )
	{
		++Shared->FailedCount;
	}
	++Shared->DoneCount;
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
		LoadSound(Set.Sounds[NextIndex - Set.Atlases.size()]);
	}
	++NextIndex;
}

void ResourceLoadJob::StepAsync()
{
	if ( !bAtlasesRequested )
	{
		bAtlasesRequested = true;
		for ( const std::string& CurrentAtlas : Set.Atlases )
		{
			const std::shared_ptr<State> Target = Shared;
			GameContext::Get().GetManager<ResourceManager>().AcquireAtlasAsync(CurrentAtlas, [Target, CurrentAtlas](AtlasHandle InHandle)
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
	}
	if ( NextSound < Set.Sounds.size() )
	{
		LoadSound(Set.Sounds[NextSound++]);
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

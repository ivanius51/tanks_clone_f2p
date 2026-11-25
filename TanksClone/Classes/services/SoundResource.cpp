#include "services/SoundResource.h"

#include <string>
#include <utility>

#include "ports/IAudioOutput.h"

namespace game
{

FSoundHandle SoundResource::Create(FPersistentString InName, FPersistentString InPath, const FSoundRegistryHandle& InRegistry)
{
	if (!InRegistry)
	{
		return nullptr;
	}

	if (SoundResource** Existing = InRegistry->Loaded.Find(InName))
	{
		if (*Existing)
		{
			return FSoundHandle(*Existing);
		}
	}

	FSoundHandle Result(new SoundResource(InName, InPath, InRegistry));
	if (InRegistry->Audio)
	{
		InRegistry->Audio->Preload(std::string(InPath.View()), nullptr);
	}
	return Result;
}

void SoundResource::CreateAsync(FPersistentString InName, FPersistentString InPath, const FSoundRegistryHandle& InRegistry, const SoundRegistry::Callback& InDone)
{
	if (!InRegistry)
	{
		if (InDone)
		{
			InDone(nullptr);
		}
		return;
	}

	if (SoundResource** Existing = InRegistry->Loaded.Find(InName))
	{
		if (*Existing)
		{
			if (InDone)
			{
				InDone(FSoundHandle(*Existing));
			}
			return;
		}
	}

	TArray<SoundRegistry::Callback>& Waiting = InRegistry->Pending[InName];
	Waiting.EmplaceBack(InDone);
	if (Waiting.Size() > 1)
	{
		return;
	}

	if (!InRegistry->Audio)
	{
		InRegistry->Pending.Remove(InName);
		if (InDone)
		{
			InDone(nullptr);
		}
		return;
	}

	// The adapter may call back synchronously (e.g. sound muted or missing file), so nothing below uses Waiting.
	InRegistry->Audio->Preload(std::string(InPath.View()), [InName, InPath, InRegistry](const bool bInSuccess)
	{
		FSoundHandle Result;
		if (bInSuccess)
		{
			if (SoundResource** Existing = InRegistry->Loaded.Find(InName))
			{
				if (*Existing)
				{
					Result = FSoundHandle(*Existing);
				}
			}

			if (!Result)
			{
				Result = FSoundHandle(new SoundResource(InName, InPath, InRegistry));
			}
		}

		TArray<SoundRegistry::Callback> Done;
		if (TArray<SoundRegistry::Callback>* PendingList = InRegistry->Pending.Find(InName))
		{
			Done = std::move(*PendingList);
			InRegistry->Pending.Remove(InName);
		}

		for (const SoundRegistry::Callback& CurrentCallback : Done)
		{
			if (CurrentCallback)
			{
				CurrentCallback(Result);
			}
		}
	});
}

SoundResource::SoundResource(FPersistentString InName, FPersistentString InPath, const FSoundRegistryHandle& InRegistry)
	: TCachedResourceBase(InName, InRegistry)
	, Path(InPath)
{

}

SoundResource::~SoundResource()
{
	if (!Registry)
	{
		return;
	}
	if (SoundResource** Existing = Registry->Loaded.Find(Name))
	{
		if (*Existing == this)
		{
			Registry->Loaded.Remove(Name);
		}
	}

	if (Registry->Audio)
	{
		Registry->Audio->Unload(std::string(Path.View()));
	}
}

} // namespace game
#pragma once

#include <cstddef>
#include <utility>

#include "model/RefCounted.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "templates/TStoredString.h"

namespace game::model
{

/** Room for a load-completion lambda that captures a name, a path and the caller's callback. */
inline constexpr std::size_t ResourceCallbackInlineSize = 128;

template <typename TDerivedResource, typename TDerivedRegistry>
struct TResourceRegistryBase : public TRefCounted
{
	using FResourceHandle = THandle<TDerivedResource>;
	using FCallback = TFunction<void(FResourceHandle), ResourceCallbackInlineSize>;

	TMap<FPersistentString, TDerivedResource*> Loaded;
	TMap<FPersistentString, TArray<FCallback>> Pending;

	FResourceHandle FindLoaded(FPersistentString InName) const noexcept
	{
		if (TDerivedResource* const* Existing = Loaded.Find(InName))
		{
			if (*Existing)
			{
				return FResourceHandle(*Existing);
			}
		}
		return nullptr;
	}

	void RegisterLoaded(FPersistentString InName, TDerivedResource* InResource)
	{
		Loaded.InsertOrAssign(InName, InResource);
	}

	void UnregisterLoaded(FPersistentString InName, const TDerivedResource* InResource)
	{
		if (TDerivedResource** Existing = Loaded.Find(InName))
		{
			if (*Existing == InResource)
			{
				Loaded.Remove(InName);
			}
		}
	}

	/** Returns true if this call started the load; false if coalesced onto an in-flight load. */
	bool EnqueueOrCoalesce(FPersistentString InName, const FCallback& InCallback)
	{
		TArray<FCallback>& Waiting = Pending[InName];
		Waiting.EmplaceBack(InCallback);
		return Waiting.Size() == 1;
	}

	void CompletePending(FPersistentString InName, FResourceHandle InResult)
	{
		TArray<FCallback> Done;
		if (TArray<FCallback>* Waiting = Pending.Find(InName))
		{
			Done = std::move(*Waiting);
			Pending.Remove(InName);
		}

		for (const FCallback& Cb : Done)
		{
			if (Cb)
			{
				Cb(InResult);
			}
		}
	}
};

template <typename TDerivedResource, typename TDerivedRegistry>
class TCachedResourceBase : public TRefCounted
{
public:
	using FRegistryHandle = THandle<TDerivedRegistry>;

	FPersistentString GetName() const noexcept { return Name; }
	const FRegistryHandle& GetRegistry() const noexcept { return Registry; }

protected:
	TCachedResourceBase(FPersistentString InName, const FRegistryHandle& InRegistry)
		: Name(InName)
		, Registry(InRegistry)
	{
		if (Registry)
		{
			Registry->RegisterLoaded(Name, static_cast<TDerivedResource*>(this));
		}
	}

	~TCachedResourceBase() override
	{
		if (Registry)
		{
			Registry->UnregisterLoaded(Name, static_cast<TDerivedResource*>(this));
		}
	}

	FPersistentString Name;
	FRegistryHandle Registry;
};

} // namespace model
#pragma once

#include "model/RefCounted.h"
#include "model/BaseResource.h"
#include "model/WorkQueue.h"
#include "templates/TFunction.h"
#include "templates/TContainers.h"
#include "templates/TStoredString.h"

namespace game
{

namespace ports
{
class IAudioOutput;
} // namespace ports

class SoundResource;
struct SoundRegistry;

using TBaseSoundRegistry = model::TResourceRegistryBase<SoundResource, SoundRegistry>;
using TBaseSoundResource = model::TCachedResourceBase<SoundResource, SoundRegistry>;
using FSoundHandle = THandle<SoundResource>;
using FSoundRegistryHandle = THandle<SoundRegistry>;

/** The sounds that are loaded now, by id name, and the requests still loading. Outlives the sounds that point to it. */
struct SoundRegistry final : public TBaseSoundRegistry
{
	using Callback = TFunction<void(FSoundHandle), model::ResourceCallbackInlineSize>;

	ports::IAudioOutput* Audio = nullptr;
};

/** One sound effect loaded into the audio engine's cache. It stays cached while a handle to it exists. */
class SoundResource final : public TBaseSoundResource
{
public:
	static FSoundHandle Create(FPersistentString InName, FPersistentString InPath, const FSoundRegistryHandle& InRegistry);
	static void CreateAsync(FPersistentString InName, FPersistentString InPath, const FSoundRegistryHandle& InRegistry, const SoundRegistry::Callback& InDone);

	FPersistentString GetPath() const noexcept { return Path; }

private:
	SoundResource(FPersistentString InName, FPersistentString InPath, const FSoundRegistryHandle& InRegistry);
	~SoundResource() override;

	FPersistentString Path;
};
} // namespace game
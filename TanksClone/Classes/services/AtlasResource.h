#pragma once

#include "model/RefCounted.h"
#include "model/BaseResource.h"
#include "model/WorkQueue.h"
#include "templates/TFunction.h"
#include "templates/TContainers.h"
#include "templates/TStoredString.h"

namespace cocos2d
{
class Texture2D;
class Image;
}

namespace game
{
class AtlasResource;
struct AtlasRegistry;

using TBaseAtlasRegistry = model::TResourceRegistryBase<AtlasResource, AtlasRegistry>;
using TBaseAtlasResource = model::TCachedResourceBase<AtlasResource, AtlasRegistry>;
using FAtlasHandle = THandle<AtlasResource>;
using FAtlasRegistryHandle = THandle<AtlasRegistry>;

/** The atlases that are loaded now, by id name, the requests still loading, and the filtering they share. Outlives the atlases that point to it. */
struct AtlasRegistry : public TBaseAtlasRegistry
{
	using Callback = TFunction<void(FAtlasHandle), model::ResourceCallbackInlineSize>;

	bool bSmoothPixels = false;
};

/**
 * One sprite atlas (.plist + .png) loaded into the engine's caches. It stays loaded while a handle to it exists
 * and unloads when the last handle goes away.
 */
class AtlasResource final : public TBaseAtlasResource
{
public:
	static FAtlasHandle Create(FPersistentString InName, FPersistentString InSdPath, const FAtlasRegistryHandle& InRegistry);
	static void CreateAsync(FPersistentString InName, FPersistentString InSdPath, const FAtlasRegistryHandle& InRegistry, model::WorkQueue& InQueue, const AtlasRegistry::Callback& InDone);

	FPersistentString GetLoadedPath() const;

	void Unload();
	void Load();
	void ApplyFiltering() const;

private:
	AtlasResource(FPersistentString InName, FPersistentString InSdPath, const FAtlasRegistryHandle& InRegistry);
	~AtlasResource() override;

	void Attach(FPersistentString InPath, cocos2d::Texture2D* InTexture);
	void Attach(FPersistentString InPath, cocos2d::Texture2D* InTexture, const std::string& InPlistText);

	FPersistentString SdPath;
	FPersistentString LoadedPath;
};
} // namespace game

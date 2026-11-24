#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "model/RefCounted.h"
#include "model/WorkQueue.h"

namespace cocos2d
{
class Texture2D;
class Image;
}

namespace tanks
{
class AtlasResource;

/** The atlases that are loaded now, by id name, the requests still loading, and the filtering they share. Outlives the atlases that point to it. */
struct AtlasRegistry
{
	using Callback = std::function<void(model::Handle<AtlasResource>)>;

	std::map<std::string, AtlasResource*> Loaded;
	std::map<std::string, std::vector<Callback>> Pending;
	bool bSmoothPixels = false;
};

/**
 * One sprite atlas (.plist + .png) loaded into the engine's caches. It stays loaded while a handle to it exists
 * and unloads when the last handle goes away.
 */
class AtlasResource final : public model::RefCounted
{
public:
	static model::Handle<AtlasResource> Create(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry);
	static void CreateAsync(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry, model::WorkQueue& InQueue, const AtlasRegistry::Callback& InDone);

	const std::string& GetName() const;
	const std::string& GetLoadedPath() const;

	void Unload();
	void Load();
	void ApplyFiltering() const;

private:
	AtlasResource(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry);
	~AtlasResource() override;

	void Attach(const std::string& InPath, cocos2d::Texture2D* InTexture);
	void Attach(const std::string& InPath, cocos2d::Texture2D* InTexture, const std::string& InPlistText);

	std::string Name;
	std::string SdPath;
	std::string LoadedPath;
	std::shared_ptr<AtlasRegistry> Registry;
};
} // namespace tanks

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "model/RefCounted.h"

namespace cocos2d
{
class Texture2D;
}

namespace tanks
{

class AtlasResource;

/** The atlases that are loaded now, by id name, the requests still loading, and the filtering they share. Outlives the atlases that point to it. */
struct AtlasRegistry
{
	using Callback = std::function<void(model::Handle<AtlasResource>)>;

	std::map<std::string, AtlasResource*> Loaded;
	// Who waits for an atlas that is being decoded in the background.
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
	// Loads the atlas of the texture set chosen now (HD or SD); an empty handle when it cannot be loaded.
	static model::Handle<AtlasResource> Create(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry);
	// Decodes the texture in the background; InDone runs later on the main thread (an empty handle on failure).
	// Several requests for one atlas share one load.
	static void CreateAsync(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry, const AtlasRegistry::Callback& InDone);

	const std::string& GetName() const;
	// The file it was loaded from (its HD or SD path); empty while unloaded.
	const std::string& GetLoadedPath() const;

	// Unload and Load again switch the atlas to the texture set chosen now (HD or SD, see DisplaySettings).
	void Unload();
	void Load();
	void ApplyFiltering() const;

private:
	AtlasResource(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry);
	~AtlasResource() override;

	// Adds the frames of a decoded texture and registers the atlas.
	void Attach(const std::string& InPath, cocos2d::Texture2D* InTexture);

	std::string Name;
	std::string SdPath;
	std::string LoadedPath;
	std::shared_ptr<AtlasRegistry> Registry;
};

} // namespace tanks

#pragma once

#include <map>
#include <memory>
#include <string>

#include "model/RefCounted.h"

namespace tanks
{

class AtlasResource;

/** The atlases that are loaded now, by id name, and the filtering they share. Outlives the atlases that point to it. */
struct AtlasRegistry
{
	std::map<std::string, AtlasResource*> Loaded;
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

	std::string Name;
	std::string SdPath;
	std::string LoadedPath;
	std::shared_ptr<AtlasRegistry> Registry;
};

} // namespace tanks

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/CCRefPtr.h"

#include "model/RefCounted.h"
#include "model/ResourceId.h"
#include "model/ResourceStatus.h"
#include "model/ResourceTable.h"
#include "model/WorkQueue.h"
#include "services/AtlasResource.h"
#include "services/SoundResource.h"

namespace cocos2d
{
class SpriteFrame;
}

namespace tanks
{

class ConfigReader;

using AtlasHandle = model::Handle<AtlasResource>;
using SoundHandle = model::Handle<SoundResource>;

/**
 * Turns resource ids into file paths using the id table of configs/resources.json (loaded on first use
 * once the configs are read; asked earlier, it logs and tries again next time; its sections may sit in other
 * config files). An unknown id or an id of another kind is logged and gives an empty path.
 * Atlases are shared through handles: an atlas is loaded when the first handle is taken and unloaded
 * when the last one is released.
 * Every resource asked for is recorded in a status book (unchecked, ok, missing). A missing one is logged once,
 * is not tried again until the texture set changes, and is replaced where possible: a sprite frame by a
 * magenta checker frame (FindFrame), a font by the system font, a sound by silence.
 */
class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	// The id for a name written in a config. An empty name gives an invalid id quietly (nothing set);
	// an unknown name or one of another kind is logged and also gives an invalid id.
	model::ResourceId FindId(std::string_view InName, model::ResourceKind InKind);
	std::string ResolvePath(const model::ResourceId& InId, model::ResourceKind InKind);

	// The atlas with this id name ("atlas.common"); loads it if no handle holds it yet. Empty handle on failure.
	AtlasHandle AcquireAtlas(std::string_view InName);
	// Like AcquireAtlas, but the texture is decoded in the background: InDone runs later on the main thread
	// (at once if the atlas is already loaded), with an empty handle on failure.
	void AcquireAtlasAsync(std::string_view InName, const AtlasRegistry::Callback& InDone);
	// Like AcquireSound, but the engine loads it in the background: InDone runs later on the main thread
	// (at once if the sound is already loaded), with an empty handle on failure.
	void AcquireSoundAsync(std::string_view InName, const SoundRegistry::Callback& InDone);
	// Reads a file on the loader thread; InDone gets its bytes on the main thread (empty if the file is missing).
	// The model for loading other kinds of data in the background, such as binary configs: do the parsing in
	// the work function of Queue.Submit and the use of the result in its done function.
	void ReadFileAsync(const std::string& InPath, const std::function<void(std::vector<unsigned char>)>& InDone);
	// Drops the loads that have not been delivered (atlases and files on the loader thread): their callbacks never
	// run. For a level change or when what they were for is gone. Sound loads run in the audio engine and are not affected.
	void CancelPendingLoads();
	// The background loader; the manager drains its finished jobs once per frame, within the time budget of
	// "pump_budget_ms" in configs/resources.json so a burst of loads does not stall one frame.
	model::WorkQueue& GetQueue();
	// The sound with this id name ("sfx.player_shot"); loads it if no handle holds it yet. Empty handle on failure.
	SoundHandle AcquireSound(std::string_view InName);
	bool IsSoundLoaded(std::string_view InName) const;
	// Atlases the whole game uses: kept loaded until the manager is destroyed.
	void KeepAtlasesLoaded(const std::vector<std::string>& InNames);
	bool IsAtlasLoaded(std::string_view InName) const;
	// The file an atlas was loaded from (its HD or SD path); empty if it is not loaded.
	std::string GetAtlasLoadedPath(std::string_view InName) const;
	// Loads every loaded atlas again from the texture set chosen now (HD or SD).
	void ReloadAtlases();
	// Sharp pixels (nearest filtering, the default) or smoothed ones, for every loaded and future atlas.
	void SetSmoothPixels(bool bInSmooth);
	bool IsSmoothPixels() const;

	const model::ResourceTable& GetTable();
	void ApplyOverrides(const model::IConfigNode& InNode);
	bool IsTableLoaded() const;
	// The configs the id table is read from; nullptr (the default) is the game's ConfigReader. For tests.
	void SetConfigReader(const ConfigReader* InReader);

	// The frame with this name, or the placeholder frame (recorded as a missing "frame", logged once).
	cocos2d::SpriteFrame* FindFrame(const std::string& InName);
	cocos2d::SpriteFrame* GetPlaceholderFrame();
	bool IsPlaceholder(const cocos2d::SpriteFrame* InFrame) const;
	// For resources loaded outside the manager (fonts): records the result; a failure is logged once.
	void ReportOk(std::string_view InKind, std::string_view InName);
	void ReportMissing(std::string_view InKind, std::string_view InName, std::string_view InDetail);
	const model::ResourceStatusBook& GetStatus() const;

	static constexpr std::string_view FrameKind = "frame";
	static constexpr std::string_view FontKind = "font";

private:
	void EnsureLoaded();
	void EnsurePumping();
	const ConfigReader& GetReader() const;
	static std::string_view KindName(model::ResourceKind InKind);
	template <typename THandle>
	THandle Record(model::ResourceKind InKind, std::string_view InName, THandle InHandle);

	model::ResourceTable Table;
	model::WorkQueue Queue;
	bool bPumping = false;
	std::chrono::microseconds PumpBudget{ 4000 };
	std::shared_ptr<AtlasRegistry> Atlases = std::make_shared<AtlasRegistry>();
	std::shared_ptr<SoundRegistry> Sounds = std::make_shared<SoundRegistry>();
	std::vector<AtlasHandle> KeptAtlases;
	bool bLoaded = false;
	const ConfigReader* Reader = nullptr;
	model::ResourceStatusBook Status;
	cocos2d::RefPtr<cocos2d::SpriteFrame> Placeholder;
};

} // namespace tanks

#pragma once

#include <chrono>
#include "templates/TFunction.h"
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
#include "templates/TFunction.h"

namespace cocos2d
{
class SpriteFrame;
}

namespace game
{
class ConfigReader;

using AtlasHandle = THandle<AtlasResource>;
using SoundHandle = THandle<SoundResource>;
using FAtlasRegistryHandle = THandle<AtlasRegistry>;
using FSoundRegistryHandle = THandle<SoundRegistry>;

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

	model::ResourceId FindId(std::string_view InName, model::ResourceKind InKind);
	FPersistentString ResolvePath(const model::ResourceId& InId, model::ResourceKind InKind);

	AtlasHandle AcquireAtlas(std::string_view InName);
	void AcquireAtlasAsync(std::string_view InName, const TFunction<void(AtlasHandle), UiCallbackInlineSize>& InDone);
	void AcquireSoundAsync(std::string_view InName, const TFunction<void(SoundHandle), UiCallbackInlineSize>& InDone);
	void ReadFileAsync(const std::string& InPath, const TFunction<void(std::vector<unsigned char>)>& InDone);
	void CancelPendingLoads();
	model::WorkQueue& GetQueue();
	SoundHandle AcquireSound(std::string_view InName);
	bool IsSoundLoaded(std::string_view InName) const;
	void KeepAtlasesLoaded(const std::vector<std::string>& InNames);
	bool IsAtlasLoaded(std::string_view InName) const;
	std::string GetAtlasLoadedPath(std::string_view InName) const;
	void ReloadAtlases();
	void SetSmoothPixels(bool bInSmooth);
	bool IsSmoothPixels() const;

	const model::ResourceTable& GetTable();
	void ApplyOverrides(const model::IConfigNode& InNode);
	bool IsTableLoaded() const;
	void SetConfigReader(const ConfigReader* InReader);

	cocos2d::SpriteFrame* FindFrame(const std::string& InName);
	cocos2d::SpriteFrame* GetPlaceholderFrame();
	bool IsPlaceholder(const cocos2d::SpriteFrame* InFrame) const;
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
	FAtlasRegistryHandle Atlases { FAtlasRegistryHandle::Create<AtlasRegistry>() };
	FSoundRegistryHandle Sounds { FSoundRegistryHandle::Create<SoundRegistry>() };
	std::vector<AtlasHandle> KeptAtlases;
	bool bLoaded = false;
	const ConfigReader* Reader = nullptr;
	model::ResourceStatusBook Status;
	cocos2d::RefPtr<cocos2d::SpriteFrame> Placeholder;
};
} // namespace game

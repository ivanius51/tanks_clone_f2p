#include "services/ResourceManager.h"

#include <algorithm>
#include <array>
#include <fstream>
#include "model/Log.h"
#include "templates/TFunction.h"
#include <iterator>

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "ports/IAudioOutput.h"
#include "templates/TContainers.h"

namespace game
{

	namespace
	{
		constexpr const char* PumpKey = "RESOURCE_QUEUE_PUMP";
		constexpr int PlaceholderSide = 16;
		constexpr int PlaceholderCell = 4;

		model::ResourceTable::ConfigResolver Resolver(const ConfigReader& InReader)
		{
			return [&InReader](const std::string& InPath) { return InReader.HasConfig(InPath) ? &InReader.GetConfig(InPath) : nullptr; };
		}

		// Magenta and black checker: impossible to mistake for real art.
		cocos2d::SpriteFrame* CreatePlaceholderFrame()
		{
			std::array<unsigned char, PlaceholderSide* PlaceholderSide * 4> Pixels = {};
			for (int Y = 0; Y < PlaceholderSide; ++Y)
			{
				for (int X = 0; X < PlaceholderSide; ++X)
				{
					const bool bMagenta = ((X / PlaceholderCell) + (Y / PlaceholderCell)) % 2 == 0;
					unsigned char* const Pixel = &Pixels[(Y * PlaceholderSide + X) * 4];
					Pixel[0] = bMagenta ? 255 : 0;
					Pixel[1] = 0;
					Pixel[2] = bMagenta ? 255 : 0;
					Pixel[3] = 255;
				}
			}
			cocos2d::Texture2D* const Texture = new (std::nothrow) cocos2d::Texture2D();
			if (!Texture)
			{
				return nullptr;
			}
			const cocos2d::Size Side(static_cast<float>(PlaceholderSide), static_cast<float>(PlaceholderSide));
			Texture->initWithData(Pixels.data(), static_cast<ssize_t>(Pixels.size()), cocos2d::Texture2D::PixelFormat::RGBA8888, PlaceholderSide, PlaceholderSide, Side);
			Texture->setAliasTexParameters();
			cocos2d::SpriteFrame* const Frame = cocos2d::SpriteFrame::createWithTexture(Texture, cocos2d::Rect(cocos2d::Vec2::ZERO, Side));
			Texture->release();
			return Frame;
		}
	}

	ResourceManager::ResourceManager()
	{
		Sounds->Audio = &GameContext::Get().GetAudioOutput();
	}

	ResourceManager::~ResourceManager()
	{
		Sounds->Audio = nullptr;
		Queue.CancelAll();
		if (bPumping)
		{
			cocos2d::Director::getInstance()->getScheduler()->unschedule(PumpKey, this);
		}
	}

	void ResourceManager::EnsurePumping()
	{
		if (bPumping)
		{
			return;
		}
		bPumping = true;
		cocos2d::Director::getInstance()->getScheduler()->schedule([this](float) { Queue.Pump(PumpBudget); }, this, 0.0f, CC_REPEAT_FOREVER, 0.0f, false, PumpKey);
	}

	void ResourceManager::CancelPendingLoads()
	{
		Queue.CancelAll();
		Atlases->Pending.Clear();
	}

	model::WorkQueue& ResourceManager::GetQueue()
	{
		EnsureLoaded();
		EnsurePumping();
		return Queue;
	}

	void ResourceManager::ReadFileAsync(const std::string& InPath, const TFunction<void(TArray<unsigned char>)>& InDone)
	{
		const std::string FullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(InPath);
		GetQueue().Submit<TArray<unsigned char>>(
			[FullPath]()
			{
				std::ifstream File(FullPath, std::ios::binary);
				return TArray<unsigned char>(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
			},
			InDone);
	}

	const ConfigReader& ResourceManager::GetReader() const
	{
		return Reader ? *Reader : GameContext::Get<ConfigReader>();
	}

	void ResourceManager::SetConfigReader(const ConfigReader* const InReader)
	{
		Reader = InReader;
		bLoaded = false;
		Table = model::ResourceTable();
	}

	bool ResourceManager::IsTableLoaded() const
	{
		return bLoaded;
	}

	void ResourceManager::EnsureLoaded()
	{
		if (bLoaded)
		{
			return;
		}
		const ConfigReader& Configs = GetReader();
		if (!Configs.HasConfig(ResourcesConfig))
		{
			model::Log::Error("ResourceManager: a resource was asked for before %s was read", ResourcesConfig);
			return;
		}
		bLoaded = true;
		const float BudgetMilliseconds = Configs.GetConfig(ResourcesConfig).GetValueOr("pump_budget_ms", 4.0f);
		PumpBudget = std::chrono::microseconds(static_cast<long long>(std::max(0.0f, BudgetMilliseconds) * 1000.0f));
		if (!Table.FromConfig(Configs.GetConfig(ResourcesConfig), Resolver(Configs)))
		{
			model::Log::Error("ResourceManager: %s has a bad or duplicated resource id", ResourcesConfig);
		}
	}

	const model::ResourceTable& ResourceManager::GetTable()
	{
		EnsureLoaded();
		return Table;
	}

	void ResourceManager::ApplyOverrides(const model::IConfigNode& InNode)
	{
		EnsureLoaded();
		Table.ApplyOverrides(InNode);
	}

	std::string_view ResourceManager::KindName(const model::ResourceKind InKind)
	{
		return model::ResourceTable::GetSectionName(InKind);
	}

	void ResourceManager::ReportOk(const std::string_view InKind, const std::string_view InName)
	{
		Status.MarkOk(InKind, InName);
	}

	void ResourceManager::ReportMissing(const std::string_view InKind, const std::string_view InName, const std::string_view InDetail)
	{
		if (Status.MarkMissing(InKind, InName))
		{
			model::Log::Info("ResourceManager: missing %s %s (%s); a placeholder is used", std::string(InKind).c_str(), std::string(InName).c_str(), std::string(InDetail).c_str());
		}
	}

	const model::ResourceStatusBook& ResourceManager::GetStatus() const
	{
		return Status;
	}

	template <typename THandle>
	THandle ResourceManager::Record(const model::ResourceKind InKind, const std::string_view InName, THandle InHandle)
	{
		if (InHandle)
		{
			ReportOk(KindName(InKind), InName);
		}
		else
		{
			ReportMissing(KindName(InKind), InName, "could not load");
		}
		return InHandle;
	}

	model::ResourceId ResourceManager::FindId(const std::string_view InName, const model::ResourceKind InKind)
	{
		if (InName.empty())
		{
			return model::ResourceId();
		}
		EnsureLoaded();
		if (!bLoaded)
		{
			return model::ResourceId();
		}
		const model::ResourceEntry* const Entry = Table.Find(InName);
		if (!Entry || Entry->Kind != InKind)
		{
			ReportMissing(KindName(InKind), InName, "unknown id");
			return model::ResourceId();
		}
		return Table.FindId(InName);
	}

	AtlasHandle ResourceManager::AcquireAtlas(const std::string_view InName)
	{
		AtlasResource* const* const Loaded = Atlases->Loaded.Find(InName);
		if (Loaded)
		{
			return AtlasHandle(*Loaded);
		}
		if (Status.IsMissing(KindName(model::ResourceKind::Atlas), InName))
		{
			return AtlasHandle();
		}
		const model::ResourceId Id = FindId(InName, model::ResourceKind::Atlas);
		if (!Id.IsValid())
		{
			return AtlasHandle();
		}
		return Record(model::ResourceKind::Atlas, InName, AtlasResource::Create(FPersistentString(InName), ResolvePath(Id, model::ResourceKind::Atlas), Atlases));
	}

	void ResourceManager::AcquireAtlasAsync(const std::string_view InName, const TFunction<void(AtlasHandle), UiCallbackInlineSize>& InDone)
	{
		AtlasResource* const* const Loaded = Atlases->Loaded.Find(InName);
		if (Loaded)
		{
			InDone(AtlasHandle(*Loaded));
			return;
		}
		const model::ResourceId Id = Status.IsMissing(KindName(model::ResourceKind::Atlas), InName) ? model::ResourceId() : FindId(InName, model::ResourceKind::Atlas);
		if (!Id.IsValid())
		{
			InDone(AtlasHandle());
			return;
		}
		const FPersistentString NameId(InName);
		const auto Pending = MakeSharedValue<const TFunction<void(AtlasHandle), UiCallbackInlineSize>>(InDone);
		AtlasResource::CreateAsync(NameId, ResolvePath(Id, model::ResourceKind::Atlas), Atlases, GetQueue(), [this, NameId, Pending](AtlasHandle InHandle)
			{
				Pending->Get()(Record(model::ResourceKind::Atlas, NameId.View(), std::move(InHandle)));
			});
	}

	SoundHandle ResourceManager::AcquireSound(const std::string_view InName)
	{
		SoundResource* const* const Loaded = Sounds->Loaded.Find(InName);
		if (Loaded)
		{
			return SoundHandle(*Loaded);
		}
		if (Status.IsMissing(KindName(model::ResourceKind::Sound), InName))
		{
			return SoundHandle();
		}
		const model::ResourceId Id = FindId(InName, model::ResourceKind::Sound);
		if (!Id.IsValid())
		{
			return SoundHandle();
		}
		return Record(model::ResourceKind::Sound, InName, SoundResource::Create(FPersistentString(InName), ResolvePath(Id, model::ResourceKind::Sound), Sounds));
	}

	void ResourceManager::AcquireSoundAsync(const std::string_view InName, const TFunction<void(SoundHandle), UiCallbackInlineSize>& InDone)
	{
		SoundResource* const* const Loaded = Sounds->Loaded.Find(InName);
		if (Loaded)
		{
			InDone(SoundHandle(*Loaded));
			return;
		}
		const model::ResourceId Id = Status.IsMissing(KindName(model::ResourceKind::Sound), InName) ? model::ResourceId() : FindId(InName, model::ResourceKind::Sound);
		if (!Id.IsValid())
		{
			InDone(SoundHandle());
			return;
		}
		const FPersistentString NameId(InName);
		const FPersistentString PathId = ResolvePath(Id, model::ResourceKind::Sound);
		const auto Pending = MakeSharedValue<const TFunction<void(SoundHandle), UiCallbackInlineSize>>(InDone);
		SoundResource::CreateAsync(NameId, PathId, Sounds, [this, NameId, PathId, Pending](SoundHandle InHandle)
			{
				// With the sound off the engine refuses every load; only a missing file is a missing sound.
				if (InHandle || !cocos2d::FileUtils::getInstance()->isFileExist(std::string(PathId.View())))
				{
					Record(model::ResourceKind::Sound, NameId.View(), InHandle);
				}
				Pending->Get()(std::move(InHandle));
			});
	}

	bool ResourceManager::IsSoundLoaded(const std::string_view InName) const
	{
		return Sounds->Loaded.Find(InName) != nullptr;
	}

	void ResourceManager::KeepAtlasesLoaded(const TArray<std::string>& InNames)
	{
		for (const std::string& CurrentName : InNames)
		{
			if (AtlasHandle Handle = AcquireAtlas(CurrentName))
			{
				KeptAtlases.PushBack(std::move(Handle));
			}
		}
	}

	bool ResourceManager::IsAtlasLoaded(const std::string_view InName) const
	{
		return Atlases->Loaded.Find(InName) != nullptr;
	}

	std::string ResourceManager::GetAtlasLoadedPath(const std::string_view InName) const
	{
		AtlasResource* const* const Loaded = Atlases->Loaded.Find(InName);
		return Loaded ? std::string((*Loaded)->GetLoadedPath().View()) : std::string();
	}

	void ResourceManager::ReloadAtlases()
	{
		// The other texture set may have what this one lacked.
		Status.ForgetMissing();
		// Sprites already made keep their old frames (they hold the textures); screens are rebuilt after a switch.
		for (const auto& CurrentAtlas : Atlases->Loaded)
		{
			CurrentAtlas.Value->Unload();
		}
		for (const auto& CurrentAtlas : Atlases->Loaded)
		{
			CurrentAtlas.Value->Load();
		}
	}

	void ResourceManager::SetSmoothPixels(const bool bInSmooth)
	{
		Atlases->bSmoothPixels = bInSmooth;
		for (const auto& CurrentAtlas : Atlases->Loaded)
		{
			CurrentAtlas.Value->ApplyFiltering();
		}
	}

	bool ResourceManager::IsSmoothPixels() const
	{
		return Atlases->bSmoothPixels;
	}

	cocos2d::SpriteFrame* ResourceManager::GetPlaceholderFrame()
	{
		if (!Placeholder)
		{
			Placeholder = CreatePlaceholderFrame();
		}
		return Placeholder.get();
	}

	bool ResourceManager::IsPlaceholder(const cocos2d::SpriteFrame* const InFrame) const
	{
		return InFrame && InFrame == Placeholder.get();
	}

	cocos2d::SpriteFrame* ResourceManager::FindFrame(const std::string& InName)
	{
		if (cocos2d::SpriteFrame* const Frame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(InName))
		{
			ReportOk(FrameKind, InName);
			return Frame;
		}
		ReportMissing(FrameKind, InName, "no atlas holds it");
		return GetPlaceholderFrame();
	}

	FPersistentString ResourceManager::ResolvePath(const model::ResourceId& InId, const model::ResourceKind InKind)
	{
		EnsureLoaded();
		if (!bLoaded)
		{
			return FPersistentString();
		}
		const model::ResourceEntry* const Entry = Table.Find(InId);
		if (!Entry || Entry->Kind != InKind)
		{
			ReportMissing(KindName(InKind), InId.GetName(), "unknown id");
			return FPersistentString();
		}
		return Entry->Path;
	}

} // namespace game

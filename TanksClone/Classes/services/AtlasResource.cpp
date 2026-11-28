#include "AtlasResource.h"

#include <fstream>
#include <iterator>
#include <new>
#include <string>
#include <utility>

#include "cocos2d.h"

#include "model/Log.h"
#include "model/RefCounted.h"
#include "services/DisplaySettings.h"
#include "templates/TStoredString.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	std::string TexturePath(const std::string& InAtlas)
	{
		const size_t Dot = InAtlas.rfind('.');
		return (Dot == std::string::npos ? InAtlas : InAtlas.substr(0, Dot)) + ".png";
	}

	bool IsPowerOfTwo(const int InValue)
	{
		return InValue > 0 && (InValue & (InValue - 1)) == 0;
	}

	// The engine builds mipmaps only for power-of-two textures. Big sets are shrunk on most screens, and trilinear mipmaps keep that smooth.
	bool CanUseMipmaps(const cocos2d::Texture2D& InTexture)
	{
		return GetTextureQuality() != ETextureQuality::Sd && IsPowerOfTwo(InTexture.getPixelsWide()) && IsPowerOfTwo(InTexture.getPixelsHigh());
	}

	void ApplyFilteringTo(cocos2d::Texture2D* const InTexture, const bool bInSmooth)
	{
		if (bInSmooth && CanUseMipmaps(*InTexture))
		{
			if (!InTexture->hasMipmaps())
			{
				InTexture->generateMipmap();
			}
			InTexture->setTexParameters({ GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE });
		}
		else if (bInSmooth)
		{
			InTexture->setAntiAliasTexParameters();
		}
		else
		{
			InTexture->setAliasTexParameters();
		}
	}

	/** What the worker thread produces: the decoded picture and the text of the frame list (the engine parses that on the main thread). */
	struct DecodedAtlas : public TRefCounted
	{
		cocos2d::RefPtr<cocos2d::Image> Image;
		std::string PlistText;
	};

	TArray<unsigned char> ReadFile(const std::string& InPath)
	{
		std::ifstream File(InPath, std::ios::binary);
		return TArray<unsigned char>(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
	}

	// Worker thread: only file reads, decoding and parsing; nothing here touches the engine's caches.
	THandle<DecodedAtlas> DecodeAtlas(const std::string& InTextureFile, const std::string& InPlistFile)
	{
		const TArray<unsigned char> Picture = ReadFile(InTextureFile);
		const TArray<unsigned char> Plist = ReadFile(InPlistFile);
		if (Picture.IsEmpty() || Plist.IsEmpty())
		{
			return nullptr;
		}

		cocos2d::Image* const Raw = new (std::nothrow) cocos2d::Image();
		if (!Raw || !Raw->initWithImageData(Picture.GetData(), static_cast<ssize_t>(Picture.Size())))
		{
			if (Raw)
			{
				Raw->release();
			}
			return nullptr;
		}

		THandle<DecodedAtlas> Result = MakeHandle<DecodedAtlas>();
		Result->Image = Raw;
		Result->PlistText.assign(reinterpret_cast<const char*>(Plist.GetData()), Plist.Size());
		Raw->release();
		return Result;
	}

	// The file of the texture set chosen now; the SD file when the HD one is missing.
	std::string ChoosePath(const std::string& InSdPath)
	{
		for ( int Level = static_cast<int>(GetTextureQuality()); Level > 0; --Level )
		{
			const std::string Path = MapAtlasPath(InSdPath, static_cast<ETextureQuality>(Level), GetTextureQualityConfig());
			if ( cocos2d::FileUtils::getInstance()->isFileExist(Path) )
			{
				return Path;
			}
			model::Log::Error("AtlasResource: no atlas %s, a smaller texture set is used", Path.c_str());
		}
		return InSdPath;
	}
} // namespace

FAtlasHandle AtlasResource::Create(FPersistentString InName, FPersistentString InSdPath, const FAtlasRegistryHandle& InRegistry)
{
	if (!InRegistry)
	{
		return nullptr;
	}

	if (AtlasResource** Existing = InRegistry->Loaded.Find(InName))
	{
		if (*Existing)
		{
			return FAtlasHandle(*Existing);
		}
	}

	FAtlasHandle Result(new AtlasResource(InName, InSdPath, InRegistry));
	Result->Load();
	return Result->LoadedPath.IsEmpty() ? FAtlasHandle() : Result;
}

void AtlasResource::CreateAsync(FPersistentString InName, FPersistentString InSdPath, const FAtlasRegistryHandle& InRegistry, model::WorkQueue& InQueue, const AtlasRegistry::Callback& InDone)
{
	if (!InRegistry)
	{
		if (InDone)
		{
			InDone(nullptr);
		}
		return;
	}

	if (AtlasResource** Existing = InRegistry->Loaded.Find(InName))
	{
		if (*Existing)
		{
			if (InDone)
			{
				InDone(FAtlasHandle(*Existing));
			}
			return;
		}
	}

	TArray<AtlasRegistry::Callback>& Waiting = InRegistry->Pending[InName];
	Waiting.EmplaceBack(InDone);
	if (Waiting.Size() > 1)
	{
		return;
	}

	// Paths are resolved here: the engine's file lookup is not safe to call from the worker thread.
	const std::string SdPathStr(InSdPath.View());
	const std::string ResolvedPath = ChoosePath(SdPathStr);
	const std::string ResolvedTexture = cocos2d::FileUtils::getInstance()->fullPathForFilename(TexturePath(ResolvedPath));
	const std::string ResolvedPlist = cocos2d::FileUtils::getInstance()->fullPathForFilename(ResolvedPath);

	const FPersistentString PathId(ResolvedPath);
	const FPersistentString TextureId(ResolvedTexture);

	InQueue.Submit<THandle<DecodedAtlas>>(
		[ResolvedTexture, ResolvedPlist]() { return DecodeAtlas(ResolvedTexture, ResolvedPlist); },
		[InName, InSdPath, PathId, TextureId, InRegistry](THandle<DecodedAtlas> InDecoded)
		{
			const std::string PathIdStr(PathId.View());
			const std::string TextureStr(TextureId.View());

			FAtlasHandle Result;
			if (AtlasResource** Existing = InRegistry->Loaded.Find(InName))
			{
				if (*Existing)
				{
					Result = FAtlasHandle(*Existing);
				}
			}

			if (!Result && InDecoded && InDecoded->Image)
			{
				if (cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(InDecoded->Image, TextureStr))
				{
					Result = FAtlasHandle(new AtlasResource(InName, InSdPath, InRegistry));
					Result->Attach(PathId, Texture, InDecoded->PlistText);
				}
			}

			if (!Result)
			{
				model::Log::Error("AtlasResource: could not load %s", PathIdStr.c_str());
			}

			TArray<AtlasRegistry::Callback> Done;
			if (TArray<AtlasRegistry::Callback>* PendingList = InRegistry->Pending.Find(InName))
			{
				Done = std::move(*PendingList);
				InRegistry->Pending.Remove(InName);
			}

			for (const AtlasRegistry::Callback& CurrentCallback : Done)
			{
				if (CurrentCallback)
				{
					CurrentCallback(Result);
				}
			}
		});
}

AtlasResource::AtlasResource(FPersistentString InName, FPersistentString InSdPath, const FAtlasRegistryHandle& InRegistry)
	: TBaseAtlasResource(InName, InRegistry)
	, SdPath(InSdPath)

{
}

AtlasResource::~AtlasResource()
{
	if (Registry)
	{
		if (AtlasResource** Existing = Registry->Loaded.Find(Name))
		{
			if (*Existing == this)
			{
				Registry->Loaded.Remove(Name);
			}
		}
	}
	Unload();
}

FPersistentString AtlasResource::GetLoadedPath() const
{
	return LoadedPath;
}

void AtlasResource::Attach(FPersistentString InPath, cocos2d::Texture2D* const InTexture)
{
	if (Registry)
	{
		ApplyFilteringTo(InTexture, Registry->bSmoothPixels);
		Registry->Loaded.InsertOrAssign(Name, this);
	}
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(std::string(InPath.View()), InTexture);
	LoadedPath = InPath;
}

void AtlasResource::Attach(FPersistentString InPath, cocos2d::Texture2D* const InTexture, const std::string& InPlistText)
{
	if (Registry)
	{
		ApplyFilteringTo(InTexture, Registry->bSmoothPixels);
		Registry->Loaded.InsertOrAssign(Name, this);
	}
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFileContent(InPlistText, InTexture);
	LoadedPath = InPath;
}

void AtlasResource::Load()
{
	const std::string Path = ChoosePath(std::string(SdPath.View()));
	cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(TexturePath(Path));
	if (!Texture)
	{
		model::Log::Error("AtlasResource: no texture for %s", Path.c_str());
		LoadedPath = FPersistentString();
		return;
	}
	Attach(FPersistentString(Path), Texture);
}

void AtlasResource::Unload()
{
	if (LoadedPath.IsEmpty())
	{
		return;
	}
	const std::string LoadedStr(LoadedPath.View());
	cocos2d::SpriteFrameCache::getInstance()->removeSpriteFramesFromFile(LoadedStr);
	cocos2d::Director::getInstance()->getTextureCache()->removeTextureForKey(TexturePath(LoadedStr));
	LoadedPath = FPersistentString();
}

void AtlasResource::ApplyFiltering() const
{
	if (LoadedPath.IsEmpty() || !Registry)
	{
		return;
	}
	const std::string Key = cocos2d::FileUtils::getInstance()->fullPathForFilename(TexturePath(std::string(LoadedPath.View())));
	if (cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->getTextureForKey(Key))
	{
		ApplyFilteringTo(Texture, Registry->bSmoothPixels);
	}
}

} // namespace game
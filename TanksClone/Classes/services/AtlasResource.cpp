#include "AtlasResource.h"

#include <fstream>
#include <iterator>
#include <cmath>
#include <new>
#include <regex>
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

	void ApplyFilteringTo(cocos2d::Texture2D* const InTexture, const bool bInSmooth)
	{
		if (bInSmooth)
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

	/** Enlarges a picture by a whole factor with nearest sampling, so SD pixel art fills the HD content scale. */
	cocos2d::Image* UpscaleImage(cocos2d::Image* const InImage, const int InFactor)
	{
		const int Width = InImage->getWidth();
		const int Height = InImage->getHeight();
		const int SourceBytes = InImage->getBitPerPixel() / 8;
		if (SourceBytes != 4 && SourceBytes != 3)
		{
			return nullptr;
		}
		constexpr int TargetBytes = 4;
		const int TargetWidth = Width * InFactor;
		const int TargetHeight = Height * InFactor;
		TArray<unsigned char> Pixels(static_cast<size_t>(TargetWidth) * TargetHeight * TargetBytes);
		const unsigned char* const Source = InImage->getData();
		for (int Y = 0; Y < TargetHeight; ++Y)
		{
			const unsigned char* const SourceRow = Source + static_cast<size_t>(Y / InFactor) * Width * SourceBytes;
			unsigned char* const TargetRow = Pixels.GetData() + static_cast<size_t>(Y) * TargetWidth * TargetBytes;
			for (int X = 0; X < TargetWidth; ++X)
			{
				const unsigned char* const Pixel = SourceRow + static_cast<size_t>(X / InFactor) * SourceBytes;
				unsigned char* const Target = TargetRow + static_cast<size_t>(X) * TargetBytes;
				Target[0] = Pixel[0];
				Target[1] = Pixel[1];
				Target[2] = Pixel[2];
				Target[3] = SourceBytes == 4 ? Pixel[3] : 255;
			}
		}
		cocos2d::Image* const Result = new (std::nothrow) cocos2d::Image();
		constexpr int BitsPerComponent = 8;
		if (!Result || !Result->initWithRawData(Pixels.GetData(), static_cast<ssize_t>(Pixels.Size()), TargetWidth, TargetHeight, BitsPerComponent, InImage->hasPremultipliedAlpha()))
		{
			if (Result)
			{
				Result->release();
			}
			return nullptr;
		}
		return Result;
	}

	/** Multiplies every number of the geometry strings ("{{x,y},{w,h}}") of a frame list by the upscale factor. */
	std::string ScalePlist(const std::string& InText, const int InFactor)
	{
		static const std::regex Geometry(R"(<string>(\{[^<]*\})</string>)");
		static const std::regex Number(R"(-?[0-9]+(\.[0-9]+)?)");
		std::string Result;
		const std::sregex_iterator Last;
		size_t Copied = 0;
		for (std::sregex_iterator It(InText.begin(), InText.end(), Geometry); It != Last; ++It)
		{
			const size_t Start = static_cast<size_t>(It->position(1));
			Result.append(InText, Copied, Start - Copied);
			const std::string Value = It->str(1);
			size_t ValueCopied = 0;
			for (std::sregex_iterator Num(Value.begin(), Value.end(), Number); Num != Last; ++Num)
			{
				Result.append(Value, ValueCopied, static_cast<size_t>(Num->position()) - ValueCopied);
				const double ScaledValue = std::stod(Num->str()) * InFactor;
				Result += std::to_string(std::llround(ScaledValue));
				ValueCopied = static_cast<size_t>(Num->position() + Num->length());
			}
			Result.append(Value, ValueCopied, std::string::npos);
			Copied = Start + Value.size();
		}
		Result.append(InText, Copied, std::string::npos);
		return Result;
	}

	// Worker thread: only file reads, decoding and parsing; nothing here touches the engine's caches.
	THandle<DecodedAtlas> DecodeAtlas(const std::string& InTextureFile, const std::string& InPlistFile, const int InUpscale)
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
		Result->PlistText.assign(reinterpret_cast<const char*>(Plist.GetData()), Plist.Size());
		cocos2d::Image* Final = Raw;
		if (InUpscale > 1)
		{
			Final = UpscaleImage(Raw, InUpscale);
			Raw->release();
			if (!Final)
			{
				return nullptr;
			}
			Result->PlistText = ScalePlist(Result->PlistText, InUpscale);
		}
		Result->Image = Final;
		Final->release();
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
	// SD files are drawn at the HD content scale, so they are enlarged at load.
	int UpscaleFor(const std::string& InChosenPath, const std::string& InSdPath)
	{
		return InChosenPath == InSdPath ? GetTextureQualityConfig().GetUpscale(ETextureQuality::Sd) : 1;
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

	const int Upscale = UpscaleFor(ResolvedPath, SdPathStr);
	const FPersistentString PathId(ResolvedPath);
	const FPersistentString TextureId(ResolvedTexture);

	InQueue.Submit<THandle<DecodedAtlas>>(
		[ResolvedTexture, ResolvedPlist, Upscale]() { return DecodeAtlas(ResolvedTexture, ResolvedPlist, Upscale); },
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
	const std::string SdPathStr(SdPath.View());
	const std::string Path = ChoosePath(SdPathStr);
	const int Upscale = UpscaleFor(Path, SdPathStr);
	if (Upscale > 1)
	{
		cocos2d::FileUtils* const Files = cocos2d::FileUtils::getInstance();
		const std::string TextureKey = Files->fullPathForFilename(TexturePath(Path));
		const THandle<DecodedAtlas> Decoded = DecodeAtlas(TextureKey, Files->fullPathForFilename(Path), Upscale);
		cocos2d::Texture2D* const Scaled = Decoded && Decoded->Image ? cocos2d::Director::getInstance()->getTextureCache()->addImage(Decoded->Image, TextureKey) : nullptr;
		if (!Scaled)
		{
			model::Log::Error("AtlasResource: no texture for %s", Path.c_str());
			LoadedPath = FPersistentString();
			return;
		}
		Attach(FPersistentString(Path), Scaled, Decoded->PlistText);
		return;
	}
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
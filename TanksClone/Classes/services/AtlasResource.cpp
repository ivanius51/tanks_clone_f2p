#include "services/AtlasResource.h"

#include <fstream>
#include <iterator>

#include "cocos2d.h"

#include "services/DisplaySettings.h"

namespace tanks
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
		if ( bInSmooth )
		{
			InTexture->setAntiAliasTexParameters();
		}
		else
		{
			InTexture->setAliasTexParameters();
		}
	}

	/** What the worker thread produces: the decoded picture and the text of the frame list (the engine parses that on the main thread). */
	struct DecodedAtlas
	{
		cocos2d::RefPtr<cocos2d::Image> Image;
		std::string PlistText;
	};

	std::vector<unsigned char> ReadFile(const std::string& InPath)
	{
		std::ifstream File(InPath, std::ios::binary);
		return std::vector<unsigned char>(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
	}

	// Worker thread: only file reads, decoding and parsing; nothing here touches the engine's caches.
	std::shared_ptr<DecodedAtlas> DecodeAtlas(const std::string& InTextureFile, const std::string& InPlistFile)
	{
		auto Result = std::make_shared<DecodedAtlas>();
		const std::vector<unsigned char> Picture = ReadFile(InTextureFile);
		const std::vector<unsigned char> Plist = ReadFile(InPlistFile);
		if ( Picture.empty() || Plist.empty() )
		{
			return Result;
		}
		cocos2d::Image* const Raw = new (std::nothrow) cocos2d::Image();
		if ( Raw && Raw->initWithImageData(Picture.data(), static_cast<ssize_t>(Picture.size())) )
		{
			Result->Image = Raw;
			Result->PlistText.assign(reinterpret_cast<const char*>(Plist.data()), Plist.size());
		}
		if ( Raw )
		{
			Raw->release();
		}
		return Result;
	}

	// The file of the texture set chosen now; the SD file when the HD one is missing.
	std::string ChoosePath(const std::string& InSdPath)
	{
		std::string Path = MapAtlasPath(InSdPath, IsHdTextures(), GetTextureQualityConfig());
		if ( Path != InSdPath && !cocos2d::FileUtils::getInstance()->isFileExist(Path) )
		{
			CCLOGERROR("AtlasResource: no HD atlas %s, the SD one is used", Path.c_str());
			Path = InSdPath;
		}
		return Path;
	}
}

model::Handle<AtlasResource> AtlasResource::Create(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry)
{
	model::Handle<AtlasResource> Result(new AtlasResource(InName, InSdPath, InRegistry));
	Result->Load();
	return Result->LoadedPath.empty() ? model::Handle<AtlasResource>() : Result;
}

void AtlasResource::CreateAsync(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry, model::WorkQueue& InQueue, const AtlasRegistry::Callback& InDone)
{
	std::vector<AtlasRegistry::Callback>& Waiting = InRegistry->Pending[InName];
	Waiting.push_back(InDone);
	if ( Waiting.size() > 1 )
	{
		return;
	}

	// Paths are resolved here: the engine's file lookup is not safe to call from the worker thread.
	const std::string Path = ChoosePath(InSdPath);
	const std::string TextureFile = cocos2d::FileUtils::getInstance()->fullPathForFilename(TexturePath(Path));
	const std::string PlistFile = cocos2d::FileUtils::getInstance()->fullPathForFilename(Path);

	InQueue.Submit<std::shared_ptr<DecodedAtlas>>(
		[TextureFile, PlistFile]() { return DecodeAtlas(TextureFile, PlistFile); },
		[InName, InSdPath, Path, TextureFile, InRegistry](std::shared_ptr<DecodedAtlas> InDecoded)
		{
			model::Handle<AtlasResource> Result;
			const auto LoadedIt = InRegistry->Loaded.find(InName);
			if ( LoadedIt != InRegistry->Loaded.end() )
			{
				Result = model::Handle<AtlasResource>(LoadedIt->second);
			}
			else if ( InDecoded && InDecoded->Image )
			{
				cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(InDecoded->Image, TextureFile);
				if ( Texture )
				{
					Result = model::Handle<AtlasResource>(new AtlasResource(InName, InSdPath, InRegistry));
					Result->Attach(Path, Texture, InDecoded->PlistText);
				}
			}
			if ( !Result )
			{
				CCLOGERROR("AtlasResource: could not load %s", Path.c_str());
			}

			std::vector<AtlasRegistry::Callback> Done = std::move(InRegistry->Pending[InName]);
			InRegistry->Pending.erase(InName);
			for ( const AtlasRegistry::Callback& CurrentCallback : Done )
			{
				CurrentCallback(Result);
			}
		});
}

AtlasResource::AtlasResource(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry)
	: Name(InName), SdPath(InSdPath), Registry(InRegistry)
{
}

AtlasResource::~AtlasResource()
{
	const auto LoadedIt = Registry->Loaded.find(Name);
	if ( LoadedIt != Registry->Loaded.end() && LoadedIt->second == this )
	{
		Registry->Loaded.erase(LoadedIt);
	}
	Unload();
}

const std::string& AtlasResource::GetName() const
{
	return Name;
}

const std::string& AtlasResource::GetLoadedPath() const
{
	return LoadedPath;
}

void AtlasResource::Attach(const std::string& InPath, cocos2d::Texture2D* const InTexture)
{
	ApplyFilteringTo(InTexture, Registry->bSmoothPixels);
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(InPath, InTexture);
	LoadedPath = InPath;
	Registry->Loaded[Name] = this;
}

void AtlasResource::Attach(const std::string& InPath, cocos2d::Texture2D* const InTexture, const std::string& InPlistText)
{
	ApplyFilteringTo(InTexture, Registry->bSmoothPixels);
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFileContent(InPlistText, InTexture);
	LoadedPath = InPath;
	Registry->Loaded[Name] = this;
}

void AtlasResource::Load()
{
	const std::string Path = ChoosePath(SdPath);
	cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(TexturePath(Path));
	if ( !Texture )
	{
		CCLOGERROR("AtlasResource: no texture for %s", Path.c_str());
		LoadedPath.clear();
		return;
	}
	Attach(Path, Texture);
}

void AtlasResource::Unload()
{
	if ( LoadedPath.empty() )
	{
		return;
	}
	cocos2d::SpriteFrameCache::getInstance()->removeSpriteFramesFromFile(LoadedPath);
	cocos2d::Director::getInstance()->getTextureCache()->removeTextureForKey(TexturePath(LoadedPath));
	LoadedPath.clear();
}

void AtlasResource::ApplyFiltering() const
{
	const std::string Key = cocos2d::FileUtils::getInstance()->fullPathForFilename(TexturePath(LoadedPath));
	if ( cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->getTextureForKey(Key) )
	{
		ApplyFilteringTo(Texture, Registry->bSmoothPixels);
	}
}

} // namespace tanks

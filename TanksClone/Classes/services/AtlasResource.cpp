#include "services/AtlasResource.h"

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

void AtlasResource::CreateAsync(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry, const AtlasRegistry::Callback& InDone)
{
	std::vector<AtlasRegistry::Callback>& Waiting = InRegistry->Pending[InName];
	Waiting.push_back(InDone);
	if ( Waiting.size() > 1 )
	{
		return;
	}

	const std::string Path = ChoosePath(InSdPath);
	// The engine may call back at once when the texture is cached, so nothing below uses Waiting.
	cocos2d::Director::getInstance()->getTextureCache()->addImageAsync(TexturePath(Path),
		[InName, InSdPath, Path, InRegistry](cocos2d::Texture2D* const InTexture)
		{
			model::Handle<AtlasResource> Result;
			const auto LoadedIt = InRegistry->Loaded.find(InName);
			if ( LoadedIt != InRegistry->Loaded.end() )
			{
				Result = model::Handle<AtlasResource>(LoadedIt->second);
			}
			else if ( InTexture )
			{
				Result = model::Handle<AtlasResource>(new AtlasResource(InName, InSdPath, InRegistry));
				Result->Attach(Path, InTexture);
			}
			else
			{
				CCLOGERROR("AtlasResource: no texture for %s", Path.c_str());
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

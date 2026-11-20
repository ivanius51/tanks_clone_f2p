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
}

model::Handle<AtlasResource> AtlasResource::Create(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry)
{
	model::Handle<AtlasResource> Result(new AtlasResource(InName, InSdPath, InRegistry));
	if ( Result->LoadedPath.empty() )
	{
		return model::Handle<AtlasResource>();
	}
	return Result;
}

AtlasResource::AtlasResource(const std::string& InName, const std::string& InSdPath, const std::shared_ptr<AtlasRegistry>& InRegistry)
	: Name(InName), SdPath(InSdPath), Registry(InRegistry)
{
	Load();
	if ( !LoadedPath.empty() )
	{
		Registry->Loaded[Name] = this;
	}
}

AtlasResource::~AtlasResource()
{
	Registry->Loaded.erase(Name);
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

void AtlasResource::Load()
{
	std::string Path = MapAtlasPath(SdPath, IsHdTextures(), GetTextureQualityConfig());
	if ( Path != SdPath && !cocos2d::FileUtils::getInstance()->isFileExist(Path) )
	{
		CCLOGERROR("AtlasResource: no HD atlas %s, the SD one is used", Path.c_str());
		Path = SdPath;
	}
	cocos2d::Texture2D* const Texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(TexturePath(Path));
	if ( !Texture )
	{
		CCLOGERROR("AtlasResource: no texture for %s", Path.c_str());
		LoadedPath.clear();
		return;
	}
	ApplyFilteringTo(Texture, Registry->bSmoothPixels);
	cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(Path, Texture);
	LoadedPath = Path;
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

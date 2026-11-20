#include "services/AtlasScope.h"

#include <algorithm>
#include <map>

#include "cocos2d.h"

#include "services/DisplaySettings.h"

namespace tanks
{

namespace
{
	struct LoadedAtlas
	{
		int Count = 0;
		std::string Path; // the HD or SD file it was loaded from
	};

	// By the atlas name the game uses (its SD path).
	std::map<std::string, LoadedAtlas>& UseCounts()
	{
		static std::map<std::string, LoadedAtlas> Counts;
		return Counts;
	}

	bool GSmoothPixels = false;

	std::string TexturePath(const std::string& InAtlas)
	{
		const size_t Dot = InAtlas.rfind('.');
		return (Dot == std::string::npos ? InAtlas : InAtlas.substr(0, Dot)) + ".png";
	}

	void ApplyFiltering(cocos2d::Texture2D* const InTexture)
	{
		if ( GSmoothPixels )
		{
			InTexture->setAntiAliasTexParameters();
		}
		else
		{
			InTexture->setAliasTexParameters();
		}
	}

	// Loads the atlas file of the current texture set; returns the file, empty if it failed.
	std::string LoadFiles(const std::string& InAtlas)
	{
		std::string Path = MapAtlasPath(InAtlas, IsHdTextures(), GetTextureQualityConfig());
		if ( Path != InAtlas && !cocos2d::FileUtils::getInstance()->isFileExist(Path) )
		{
			CCLOGERROR("AtlasScope: no HD atlas %s, the SD one is used", Path.c_str());
			Path = InAtlas;
		}
		cocos2d::Texture2D* Texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(TexturePath(Path));
		if ( !Texture )
		{
			CCLOGERROR("AtlasScope: no texture for %s", Path.c_str());
			return std::string();
		}
		ApplyFiltering(Texture);
		cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(Path, Texture);
		return Path;
	}

	void UnloadFiles(const std::string& InPath)
	{
		cocos2d::SpriteFrameCache::getInstance()->removeSpriteFramesFromFile(InPath);
		cocos2d::Director::getInstance()->getTextureCache()->removeTextureForKey(TexturePath(InPath));
	}

	bool Acquire(const std::string& InAtlas)
	{
		LoadedAtlas& Current = UseCounts()[InAtlas];
		if ( Current.Count == 0 )
		{
			Current.Path = LoadFiles(InAtlas);
			if ( Current.Path.empty() )
			{
				UseCounts().erase(InAtlas);
				return false;
			}
		}
		++Current.Count;
		return true;
	}

	void Release(const std::string& InAtlas)
	{
		auto CountIt = UseCounts().find(InAtlas);
		if ( CountIt == UseCounts().end() || --CountIt->second.Count > 0 )
		{
			return;
		}
		const std::string Path = CountIt->second.Path;
		UseCounts().erase(CountIt);
		UnloadFiles(Path);
	}
}

AtlasScope::AtlasScope(const std::vector<std::string>& InAtlases)
{
	for ( const auto& CurrentAtlas : InAtlases )
	{
		Add(CurrentAtlas);
	}
}

AtlasScope::~AtlasScope()
{
	for ( const auto& CurrentAtlas : Atlases )
	{
		Release(CurrentAtlas);
	}
}

bool AtlasScope::Add(const std::string& InAtlas)
{
	if ( std::find(Atlases.begin(), Atlases.end(), InAtlas) != Atlases.end() )
	{
		return true;
	}
	if ( !Acquire(InAtlas) )
	{
		return false;
	}
	Atlases.push_back(InAtlas);
	return true;
}

const std::vector<std::string>& AtlasScope::GetAtlases() const
{
	return Atlases;
}

void AtlasScope::KeepLoaded(const std::vector<std::string>& InAtlases)
{
	for ( const auto& CurrentAtlas : InAtlases )
	{
		if ( !IsLoaded(CurrentAtlas) )
		{
			Acquire(CurrentAtlas);
		}
	}
}

void AtlasScope::SetSmoothPixels(const bool bInSmooth)
{
	GSmoothPixels = bInSmooth;
	auto Cache = cocos2d::Director::getInstance()->getTextureCache();
	for ( const auto& CurrentAtlas : UseCounts() )
	{
		if ( cocos2d::Texture2D* Texture = Cache->getTextureForKey(cocos2d::FileUtils::getInstance()->fullPathForFilename(TexturePath(CurrentAtlas.second.Path))) )
		{
			ApplyFiltering(Texture);
		}
	}
}

bool AtlasScope::IsSmoothPixels()
{
	return GSmoothPixels;
}

bool AtlasScope::IsLoaded(const std::string& InAtlas)
{
	auto CountIt = UseCounts().find(InAtlas);
	return CountIt != UseCounts().end() && CountIt->second.Count > 0;
}

void AtlasScope::ReloadAll()
{
	// Sprites already made keep their old frames (they hold the textures); screens are rebuilt after a switch.
	for ( auto& CurrentAtlas : UseCounts() )
	{
		UnloadFiles(CurrentAtlas.second.Path);
	}
	for ( auto& CurrentAtlas : UseCounts() )
	{
		const std::string Path = LoadFiles(CurrentAtlas.first);
		CurrentAtlas.second.Path = Path.empty() ? CurrentAtlas.first : Path;
	}
}

std::string AtlasScope::GetLoadedPath(const std::string& InAtlas)
{
	auto CountIt = UseCounts().find(InAtlas);
	return CountIt != UseCounts().end() ? CountIt->second.Path : std::string();
}

} // namespace tanks

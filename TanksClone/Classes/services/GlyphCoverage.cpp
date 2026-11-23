#include "services/GlyphCoverage.h"

#include <set>

#include "cocos2d.h"
#include "ft2build.h"
#include FT_FREETYPE_H

#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/ResourceId.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"

namespace tanks
{

std::vector<std::string> GlyphCoverage::FindMissing(const std::string_view InLanguageId, const std::string_view InFontId)
{
	std::vector<std::string> Missing;
	const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(model::ResourceId(InFontId), model::ResourceKind::Font);
	const cocos2d::Data FontData = Path.empty() ? cocos2d::Data() : cocos2d::FileUtils::getInstance()->getDataFromFile(Path);
	if ( FontData.isNull() )
	{
		return Missing;
	}

	FT_Library Library = nullptr;
	FT_Face Face = nullptr;
	if ( FT_Init_FreeType(&Library) != 0 )
	{
		return Missing;
	}
	// The face reads straight from the loaded bytes, which outlive it.
	if ( FT_New_Memory_Face(Library, FontData.getBytes(), static_cast<FT_Long>(FontData.getSize()), 0, &Face) == 0 )
	{
		std::set<char32_t> Seen;
		for ( const std::string& Text : GameContext::Get().GetManager<StringsManager>().GetAllTexts(InLanguageId) )
		{
			std::u32string Letters;
			cocos2d::StringUtils::UTF8ToUTF32(Text, Letters);
			for ( const char32_t Letter : Letters )
			{
				if ( Letter > U' ' && Seen.insert(Letter).second && FT_Get_Char_Index(Face, static_cast<FT_ULong>(Letter)) == 0 )
				{
					std::string Utf8;
					cocos2d::StringUtils::UTF32ToUTF8(std::u32string(1, Letter), Utf8);
					Missing.push_back(Utf8);
				}
			}
		}
		FT_Done_Face(Face);
	}
	FT_Done_FreeType(Library);
	return Missing;
}

void GlyphCoverage::ReportAll()
{
	const StringsManager& Strings = GameContext::Get().GetManager<StringsManager>();
	for ( const LanguageDef& Language : Strings.GetLanguages() )
	{
		for ( const bool bDecorative : { true, false } )
		{
			const std::string& Override = bDecorative ? Language.DecorativeFontId : Language.PlainFontId;
			const std::string_view FontId = Override.empty() ? (bDecorative ? ids::fonts::Decorative : ids::fonts::Plain).GetName() : std::string_view(Override);
			for ( const std::string& Letter : FindMissing(Language.Id, FontId) )
			{
				cocos2d::log("GlyphCoverage: font %s has no glyph '%s' used by language %s", std::string(FontId).c_str(), Letter.c_str(), Language.Id.c_str());
			}
		}
	}
}

} // namespace tanks

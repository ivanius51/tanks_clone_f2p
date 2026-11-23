#include "services/StringsManager.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"

namespace tanks
{

void StringsManager::Init()
{
	auto& Configs = GameContext::Get().GetManager<ConfigReader>();
	Languages.clear();
	Strings.clear();

	const model::IConfigNode& Root = Configs.GetConfig(config_refs::ResolveId(ids::configs::Languages));
	const model::IConfigNode* const Items = Root.FindChild("languages");
	for ( std::size_t Index = 0; Items && Index < Items->GetItemCount(); ++Index )
	{
		const model::IConfigNode& Item = *Items->GetItem(Index);
		LanguageDef Language;
		Language.Id = Item.GetString("id");
		Language.Name = Item.GetValueOr("name", Language.Id.c_str());
		Language.StringsId = Item.GetString("strings");
		if ( const model::IConfigNode* const Fonts = Item.FindChild("fonts") )
		{
			Language.DecorativeFontId = Fonts->GetValueOr("decorative", "");
			Language.PlainFontId = Fonts->GetValueOr("plain", "");
		}
		const std::string StringsPath = config_refs::ResolveName(Language.StringsId);
		if ( Language.Id.empty() || StringsPath.empty() || FindLanguage(Language.Id) )
		{
			cocos2d::log("StringsManager: language entry %d skipped (empty or repeated id, or unknown strings file)", static_cast<int>(Index));
			continue;
		}
		Strings[Language.Id] = Configs.GetObjectData(StringsPath);
		Languages.push_back(std::move(Language));
	}

	DefaultLanguageId = Root.GetValueOr("default", "");
	if ( !FindLanguage(DefaultLanguageId) )
	{
		cocos2d::log("StringsManager: default language '%s' is not in the list", DefaultLanguageId.c_str());
		DefaultLanguageId = Languages.empty() ? std::string() : Languages.front().Id;
	}

	for ( const LanguageDef& Language : Languages )
	{
		for ( const std::string& Key : FindMissingKeys(Language.Id) )
		{
			cocos2d::log("StringsManager: language %s lacks the string %s", Language.Id.c_str(), Key.c_str());
		}
	}
	SetLanguage(CurrentLanguageId.empty() ? DefaultLanguageId : CurrentLanguageId);
}

const LanguageDef* StringsManager::FindLanguage(const std::string_view InLanguageId) const
{
	for ( const LanguageDef& Language : Languages )
	{
		if ( Language.Id == InLanguageId )
		{
			return &Language;
		}
	}
	return nullptr;
}

const cocos2d::ValueMap* StringsManager::FindStrings(const std::string_view InLanguageId) const
{
	const auto Found = Strings.find(InLanguageId);
	return Found == Strings.end() ? nullptr : &Found->second.Values;
}

std::string StringsManager::GetString(const std::string& InStringId) const
{
	for ( const std::string* const Language : { &CurrentLanguageId, &DefaultLanguageId } )
	{
		const cocos2d::ValueMap* const Texts = FindStrings(*Language);
		if ( !Texts )
		{
			continue;
		}
		const auto Found = Texts->find(InStringId);
		if ( Found != Texts->end() )
		{
			return Found->second.asString();
		}
	}
	cocos2d::log("StringsManager: string not found, StringId - %s", InStringId.c_str());
	return InStringId;
}

void StringsManager::SetLanguage(const std::string_view InLanguageId)
{
	if ( Languages.empty() || FindLanguage(InLanguageId) )
	{
		CurrentLanguageId = std::string(InLanguageId);
		return;
	}
	if ( !InLanguageId.empty() )
	{
		cocos2d::log("StringsManager: unknown language '%s', using '%s'", std::string(InLanguageId).c_str(), DefaultLanguageId.c_str());
	}
	CurrentLanguageId = DefaultLanguageId;
}

std::string StringsManager::GetNextLanguageId(const std::string_view InLanguageId) const
{
	for ( std::size_t Index = 0; Index < Languages.size(); ++Index )
	{
		if ( Languages[Index].Id == InLanguageId )
		{
			return Languages[(Index + 1) % Languages.size()].Id;
		}
	}
	return DefaultLanguageId;
}

const std::string& StringsManager::GetFontId(const bool bInDecorative) const
{
	static const std::string None;
	const LanguageDef* const Current = FindLanguage(CurrentLanguageId);
	if ( !Current )
	{
		return None;
	}
	return bInDecorative ? Current->DecorativeFontId : Current->PlainFontId;
}

std::vector<std::string> StringsManager::FindMissingKeys(const std::string_view InLanguageId) const
{
	std::vector<std::string> Missing;
	const cocos2d::ValueMap* const Reference = FindStrings(DefaultLanguageId);
	const cocos2d::ValueMap* const Texts = FindStrings(InLanguageId);
	if ( !Reference || !Texts )
	{
		return Missing;
	}
	for ( const auto& Entry : *Reference )
	{
		if ( Texts->count(Entry.first) == 0 )
		{
			Missing.push_back(Entry.first);
		}
	}
	return Missing;
}

std::vector<std::string> StringsManager::GetAllTexts(const std::string_view InLanguageId) const
{
	std::vector<std::string> Texts;
	if ( const cocos2d::ValueMap* const Values = FindStrings(InLanguageId) )
	{
		for ( const auto& Entry : *Values )
		{
			Texts.push_back(Entry.second.asString());
		}
	}
	return Texts;
}

} // namespace tanks

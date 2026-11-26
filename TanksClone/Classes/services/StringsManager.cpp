#include "services/StringsManager.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"

namespace game
{

void StringsManager::Init()
{
	auto& Configs = GameContext::Get<ConfigReader>();
	Languages.clear();
	Strings.Clear();

	const model::IConfigNode& Root = Configs.GetConfig(config_refs::ResolveId(ids::configs::Languages));
	const model::IConfigNode* const Items = Root.FindChild("languages");
	for ( std::size_t Index = 0; Items && Index < Items->GetItemCount(); ++Index )
	{
		const model::IConfigNode& Item = *Items->GetItem(Index);
		const std::string IdText = Item.GetString("id");
		LanguageDef Language;
		Language.Id = FPersistentString(IdText);
		Language.Name = Item.GetValueOr("name", IdText.c_str());
		Language.StringsId = FPersistentString(Item.GetString("strings"));
		if ( const model::IConfigNode* const Fonts = Item.FindChild("fonts") )
		{
			Language.DecorativeFontId = FPersistentString(Fonts->GetValueOr("decorative", ""));
			Language.PlainFontId = FPersistentString(Fonts->GetValueOr("plain", ""));
		}
		const FPersistentString StringsPath = config_refs::ResolveName(Language.StringsId.View());
		if ( Language.Id.IsEmpty() || StringsPath.IsEmpty() || FindLanguage(Language.Id.View()) )
		{
			cocos2d::log("StringsManager: language entry %d skipped (empty or repeated id, or unknown strings file)", static_cast<int>(Index));
			continue;
		}
		Strings[Language.Id] = Configs.GetObjectData(StringsPath);
		Languages.push_back(std::move(Language));
	}

	DefaultLanguageId = FPersistentString(Root.GetValueOr("default", ""));
	if ( !FindLanguage(DefaultLanguageId.View()) )
	{
		cocos2d::log("StringsManager: default language '%s' is not in the list", std::string(DefaultLanguageId.View()).c_str());
		DefaultLanguageId = Languages.empty() ? FPersistentString() : Languages.front().Id;
	}

	for ( const LanguageDef& Language : Languages )
	{
		for ( const std::string& Key : FindMissingKeys(Language.Id.View()) )
		{
			cocos2d::log("StringsManager: language %s lacks the string %s", std::string(Language.Id.View()).c_str(), Key.c_str());
		}
	}
	SetLanguage(CurrentLanguageId.IsEmpty() ? DefaultLanguageId.View() : CurrentLanguageId.View());
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

const cocos2d::ValueMap* StringsManager::FindStrings(const FPersistentString InLanguageId) const
{
	auto* const Found = Strings.Find(InLanguageId);
	return !Found ? nullptr : &Found->Values;
}

std::string StringsManager::GetString(const std::string& InStringId) const
{
	for ( const FPersistentString Language : { CurrentLanguageId, DefaultLanguageId } )
	{
		const cocos2d::ValueMap* const Texts = FindStrings(Language);
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
	if ( const LanguageDef* const Found = FindLanguage(InLanguageId) )
	{
		CurrentLanguageId = Found->Id;
		return;
	}
	if ( !InLanguageId.empty() )
	{
		cocos2d::log("StringsManager: unknown language '%s', using '%s'", std::string(InLanguageId).c_str(), std::string(DefaultLanguageId.View()).c_str());
	}
	CurrentLanguageId = DefaultLanguageId;
}

std::string StringsManager::GetNextLanguageId(const std::string_view InLanguageId) const
{
	for ( std::size_t Index = 0; Index < Languages.size(); ++Index )
	{
		if ( Languages[Index].Id == InLanguageId )
		{
			return std::string(Languages[(Index + 1) % Languages.size()].Id.View());
		}
	}
	return std::string(DefaultLanguageId.View());
}

FPersistentString StringsManager::GetFontId(const bool bInDecorative) const
{
	const LanguageDef* const Current = FindLanguage(CurrentLanguageId.View());
	if ( !Current )
	{
		return FPersistentString();
	}
	return bInDecorative ? Current->DecorativeFontId : Current->PlainFontId;
}

std::vector<std::string> StringsManager::FindMissingKeys(const std::string_view InLanguageId) const
{
	std::vector<std::string> Missing;
	const cocos2d::ValueMap* const Reference = FindStrings(DefaultLanguageId);
	const cocos2d::ValueMap* const Texts = FindStrings(FPersistentString::Find(InLanguageId));
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

std::vector<std::string> StringsManager::GetAllKeys() const
{
	std::vector<std::string> Keys;
	if ( const cocos2d::ValueMap* const Values = FindStrings(DefaultLanguageId) )
	{
		for ( const auto& Entry : *Values )
		{
			Keys.push_back(Entry.first);
		}
	}
	std::sort(Keys.begin(), Keys.end());
	return Keys;
}

std::vector<std::string> StringsManager::GetAllTexts(const std::string_view InLanguageId) const
{
	std::vector<std::string> Texts;
	if ( const cocos2d::ValueMap* const Values = FindStrings(FPersistentString::Find(InLanguageId)) )
	{
		for ( const auto& Entry : *Values )
		{
			Texts.push_back(Entry.second.asString());
		}
	}
	return Texts;
}

} // namespace game

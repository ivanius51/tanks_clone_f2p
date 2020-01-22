#include "StringsManager.h"

NS_CC_BEGIN

StringsManager::StringsManager()
	: mCurrentLanguage( eLanguage::RUSSIAN )
{
}

StringsManager::~StringsManager()
{
}

void StringsManager::loadLanguageConfig( eLanguage aLanguage, const std::string& aPath )
{
	// read language config
}

std::string StringsManager::getString( const std::string& aStringId )
{
	std::string result = aStringId;

	auto findStringsMap = mStrings.find( mCurrentLanguage );
	if ( findStringsMap != mStrings.end() )
	{
		const auto& stringsMap = findStringsMap->second;
		auto findString = stringsMap.find( aStringId );
		if ( findString != stringsMap.end() )
		{
			result = findString->second;
		}
		else
		{
			log( "StringsManager: string not found, StringId - %s", aStringId.c_str() );
		}
	}
	else
	{
		log( "StringsManager: language strings map not found, Language - %d", mCurrentLanguage );
	}

	return result;
}

void StringsManager::setLanguage( eLanguage aLanguage )
{
	if ( mCurrentLanguage != aLanguage )
	{
		mCurrentLanguage = aLanguage;

		//notify game on change language
	}
}

NS_CC_END
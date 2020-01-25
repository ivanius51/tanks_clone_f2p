#include "StringsManager.h"
#include "ConfigReader.h"

NS_CC_BEGIN

StringsManager::StringsManager()
	: mCurrentLanguage( eLanguage::RUSSIAN )
{
}

StringsManager::~StringsManager()
{
}

StringsManager& StringsManager::getInstance()
{
	static StringsManager instance;
	return instance;
}

void StringsManager::init()
{
	mStrings[ eLanguage::ENGLISH ] = CONFIGS.getObjectData( "configs\\strings\\english.json" );
	mStrings[ eLanguage::RUSSIAN ] = CONFIGS.getObjectData( "configs\\strings\\russian.json" );
}

std::string StringsManager::getString( const std::string& aStringId )
{
	std::string result = aStringId;

	auto findStringsMap = mStrings.find( mCurrentLanguage );
	if ( findStringsMap != mStrings.end() )
	{
		const auto& stringsMap = findStringsMap->second;
		auto findString = stringsMap.data.find( aStringId );
		if ( findString != stringsMap.data.end() )
		{
			result = findString->second.asString();
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
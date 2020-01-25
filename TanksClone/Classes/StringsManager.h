#ifndef _StringsManager_H_
#define _StringsManager_H_

#include "cocos2d.h"
#include "ObjectTypes.h"

NS_CC_BEGIN

enum class eLanguage
{
	NONE = 0,
	RUSSIAN,
	ENGLISH
};

class StringsManager
{
private:
	
	std::map< eLanguage, sObjectConfig > mStrings;
	eLanguage mCurrentLanguage;

	StringsManager();
	virtual ~StringsManager();

public:

	static StringsManager& getInstance();

	void init();

	std::string getString( const std::string& aStringId );
	void setLanguage( eLanguage aLanguage );
};

#define STRINGS StringsManager::getInstance()

NS_CC_END

#endif

#ifndef _StringsManager_H_
#define _StringsManager_H_

#include "cocos2d.h"

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
	
	std::map< eLanguage, std::map< std::string, std::string > > mStrings;
	eLanguage mCurrentLanguage;

public:

	StringsManager();
	virtual ~StringsManager();

	void loadLanguageConfig( eLanguage aLanguage, const std::string& aPath );

	std::string getString( const std::string& aStringId );
	void setLanguage( eLanguage aLanguage );
};

NS_CC_END

#endif

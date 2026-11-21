#include "platform/cocos/CocosStorage.h"

#include "cocos2d.h"

namespace tanks::view::cocos
{

std::string CocosStorage::PathOf(const std::string& InName)
{
	return cocos2d::FileUtils::getInstance()->getWritablePath() + InName;
}

bool CocosStorage::Exists(const std::string& InName) const
{
	return cocos2d::FileUtils::getInstance()->isFileExist(PathOf(InName));
}

std::optional<std::string> CocosStorage::ReadText(const std::string& InName) const
{
	if ( !Exists(InName) )
	{
		return std::nullopt;
	}
	return cocos2d::FileUtils::getInstance()->getStringFromFile(PathOf(InName));
}

bool CocosStorage::WriteText(const std::string& InName, const std::string& InText)
{
	return cocos2d::FileUtils::getInstance()->writeStringToFile(InText, PathOf(InName));
}

} // namespace tanks::view::cocos

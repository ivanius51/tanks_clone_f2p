#ifndef _DataConverter_H_
#define _DataConverter_H_

#include "cocos2d.h"
#include "ObjectTypes.h"

NS_CC_BEGIN
class DataConverter
{
public:

	static Vec2 ValueToVec2(const Value& aValue);
	static Color3B ValueToColor3B(const Value& aValue);
	static std::map<std::string, std::string> ValueToStringMap(const Value& aValue);

};

NS_CC_END

#endif
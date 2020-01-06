#ifndef _GameCommand
#define _GameCommand

#include "cocos2d/cocos/base/CCValue.h"
#include "ObjectTypes.h"

NS_CC_BEGIN
class GameCommand
{
private:
	ValueMap mData;
	std::string mTarget;
	eCommandType mType;
public:
	GameCommand(eCommandType aType = eCommandType::COMMAND_TYPE_UNDEFINED, std::string aTarget = "", ValueMap aData = ValueMap());

	const ValueMap& getData() const;
	const std::string& getTarget() const;
	const eCommandType& getType() const;
};
NS_CC_END
#endif


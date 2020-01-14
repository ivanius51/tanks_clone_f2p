#ifndef _GameCommand_H_
#define _GameCommand_H_

#include "cocos2d.h"
#include "ObjectTypes.h"

NS_CC_BEGIN
class GameCommand
{
private:
	ValueMap mData;
	std::string mTarget;
	eObjectType mTargetType;
	eCommandType mType;

public:
	GameCommand(eCommandType aType = eCommandType::COMMAND_TYPE_UNDEFINED, std::string aTarget = "", ValueMap aData = ValueMap());
	GameCommand(eCommandType aType = eCommandType::COMMAND_TYPE_UNDEFINED, eObjectType aTarget = eObjectType::OBJECT_TYPE_UNDEFINED, ValueMap aData = ValueMap());

	void setTarget(const std::string& aTarget);
	void setTargetType(eObjectType aTargetType);

	void setData(const std::string& aKey, const Value& aValue);

	const ValueMap& getData() const;
	const std::string& getTarget() const;
	const eObjectType& getTargetType() const;
	const eCommandType& getType() const;
};
NS_CC_END
#endif


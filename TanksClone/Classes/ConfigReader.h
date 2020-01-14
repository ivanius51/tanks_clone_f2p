#ifndef _ConfigReader_H_
#define _ConfigReader_H_

#include "cocos2d.h"
#include "ObjectTypes.h"
#include "external/json/document.h"

NS_CC_BEGIN
class ConfigReader
{
private:
	Value parseValueFromJsonValue(const rapidjson::Value& aJsonValue);
	sObjectConfig readConfig(const std::string& aPath);

public:
	ConfigReader();
	~ConfigReader();

	const sNodeConfig& getNodeData(const std::string& aId);
	const sObjectConfig& getObjectData(const std::string& aId);
	
	void readObjectConfigs (const std::string& aFolderPath);
};

NS_CC_END

#endif
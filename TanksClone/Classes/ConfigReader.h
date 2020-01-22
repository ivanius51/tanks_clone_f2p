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
	sObjectConfig readConfigByPath(const std::string& aPath);
	sObjectConfig readConfig(const std::string& aData);

	std::map < std::string, sObjectConfig > mConfigsData;

public:
	ConfigReader();
	~ConfigReader();
	static ConfigReader& getInstance();

	const sNodeConfig& getNodeData(const std::string& aId);
	const sObjectConfig& getObjectData(const std::string& aId);
	
	void readObjectConfigs (const std::string& aFolderPath);


};

#define CONFIGS ConfigReader::getInstance()

NS_CC_END

#endif
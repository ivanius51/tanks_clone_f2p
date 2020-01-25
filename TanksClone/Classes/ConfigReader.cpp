#include "ConfigReader.h"

#include "external/json/reader.h"
#include "external/json/writer.h"
#include "external/json/stringbuffer.h"
#include "external/json/prettywriter.h"

NS_CC_BEGIN


ConfigReader::ConfigReader()
{
}


Value ConfigReader::parseValueFromJsonValue(const rapidjson::Value& aJsonValue)
{
	Value result;
	if ( aJsonValue.IsObject() )
	{
		ValueMap valueMap;

		for ( auto it = aJsonValue.MemberBegin(); it != aJsonValue.MemberEnd(); ++it )
		{
			valueMap[(*it).name.GetString()] = parseValueFromJsonValue((*it).value);
		}
		result = valueMap;
	}
	else if ( aJsonValue.IsArray() )
	{
		ValueVector valueVec;

		for ( int i = 0; i != aJsonValue.Size(); ++i )
		{
			valueVec.push_back(parseValueFromJsonValue(aJsonValue[i]));
		}

		result = valueVec;
	}
	else if ( aJsonValue.IsFloat() || aJsonValue.IsDouble() )
	{
		result = aJsonValue.GetDouble();
	}
	else if ( aJsonValue.IsBool() )
	{
		result = aJsonValue.GetBool();
	}
	else if ( aJsonValue.IsString() )
	{
		result = aJsonValue.GetString();
	}
	else if ( aJsonValue.IsNumber() )
	{
		result = aJsonValue.GetInt();
	}
	return result;
}

sObjectConfig ConfigReader::readConfigByPath(const std::string& aPath)
{
	sObjectConfig result;

	std::string content = FileUtils::getInstance()->getStringFromFile(aPath);
	if ( !content.empty() )
	{
		result = readConfig(content);
	}
	return result;
}
sObjectConfig ConfigReader::readConfig(const std::string& aPath)
{
	sObjectConfig result;
	//std::string clearContent = aPath.substr(0, aPath.rfind('}') + 1);

	rapidjson::Document document;
	document.Parse<0>(aPath.c_str());
	if ( !document.HasParseError()
		&& document.IsObject()
		)
	{
		result.data = parseValueFromJsonValue(document).asValueMap();
	}
	return result;
}

ConfigReader::~ConfigReader()
{
}

ConfigReader& ConfigReader::getInstance()
{
	static ConfigReader instance;
	return instance;
}

void ConfigReader::readObjectConfigs(const std::string& aFolderPath)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	auto fileUtils = FileUtils::getInstance();
	std::string fullPath = fileUtils->getDefaultResourceRootPath() + aFolderPath;
	fileUtils->addSearchPath(fullPath);
	fullPath += '*';

	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFileA(fullPath.c_str(), &FindFileData);
	while ( hFind != INVALID_HANDLE_VALUE )
	{
		auto fileName = std::string(FindFileData.cFileName);
		if ( (fileName != ".")
			&& (fileName != "..") )
		{
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				readObjectConfigs(aFolderPath + fileName + "\\");
			}
			else
			{
				mConfigsData[aFolderPath + FindFileData.cFileName] = readConfig(fileUtils->getStringFromFile(FindFileData.cFileName));
			}
		}

		if ( !FindNextFileA(hFind, &FindFileData) )
		{
			break;
		}
	}
	FindClose(hFind);
#endif
}

NS_CC_END

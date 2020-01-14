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
		ValueMap& valueMap = result.asValueMap();

		for ( auto it = aJsonValue.MemberBegin(); it != aJsonValue.MemberEnd(); ++it )
		{
			valueMap[(*it).name.GetString()] = parseValueFromJsonValue((*it).value);
		}
	}
	else if ( aJsonValue.IsArray() )
	{
		ValueVector& valueVec = result.asValueVector();

		for ( int i = 0; i != aJsonValue.Size(); ++i )
		{
			valueVec.push_back(parseValueFromJsonValue(aJsonValue[i]));
		}
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

sObjectConfig ConfigReader::readConfig(const std::string& aPath)
{
	sObjectConfig result;

	std::string content = FileUtils::getInstance()->getStringFromFile(aPath);
	if ( !content.empty() )
	{
		std::string clearContent = content.substr(0, content.rfind('}') + 1);

		rapidjson::Document document;
		document.Parse<0>(clearContent.c_str());
		if ( !document.HasParseError()
			&& document.IsObject()
			)
		{
			if ( document.MemberBegin() != document.MemberEnd() )
			{
				result.data = parseValueFromJsonValue(document.MemberBegin()->value).asValueMap();
			}
		}
	}


	return result;
}

ConfigReader::~ConfigReader()
{
}

void ConfigReader::readObjectConfigs(const std::string& aFolderPath)
{

}

NS_CC_END

#include "core/ConfigReader.h"

#include <algorithm>
#include <string_view>
#include <vector>
#include "external/json/document.h"
#include "core/JsonConfigNode.h"

namespace tanks
{

namespace
{

cocos2d::Value ParseJsonValue(const rapidjson::Value& InJsonValue)
{
	if ( InJsonValue.IsObject() )
	{
		cocos2d::ValueMap CurrentValueMap;
		for ( auto It = InJsonValue.MemberBegin(); It != InJsonValue.MemberEnd(); ++It )
		{
			CurrentValueMap[It->name.GetString()] = ParseJsonValue(It->value);
		}
		return cocos2d::Value(CurrentValueMap);
	}
	if ( InJsonValue.IsArray() )
	{
		cocos2d::ValueVector ValueVec;
		for ( rapidjson::SizeType Index = 0; Index < InJsonValue.Size(); ++Index )
		{
			ValueVec.push_back(ParseJsonValue(InJsonValue[Index]));
		}
		return cocos2d::Value(ValueVec);
	}
	if ( InJsonValue.IsBool() )
	{
		return cocos2d::Value(InJsonValue.GetBool());
	}
	if ( InJsonValue.IsString() )
	{
		return cocos2d::Value(InJsonValue.GetString());
	}
	if ( InJsonValue.IsInt() )
	{
		return cocos2d::Value(InJsonValue.GetInt());
	}
	if ( InJsonValue.IsNumber() )
	{
		return cocos2d::Value(InJsonValue.GetDouble());
	}
	return cocos2d::Value(); // null
}

ObjectConfig ParseConfig(const std::string& InJsonText, const std::string& InId)
{
	ObjectConfig Result;

	rapidjson::Document Document;
	Document.Parse<0>(InJsonText.c_str());
	if ( Document.HasParseError() || !Document.IsObject() )
	{
		CCLOGERROR("ConfigReader: failed to parse %s", InId.c_str());
		return Result;
	}

	Result.Values = ParseJsonValue(Document).asValueMap();
	Result.Node = JsonConfigNode::Parse(InJsonText, InId);
	return Result;
}

std::string NormalizeSlashes(std::string InPath)
{
	std::replace(InPath.begin(), InPath.end(), '\\', '/');
	return InPath;
}

bool EndsWith(std::string_view InText, std::string_view InSuffix)
{
	return InText.size() >= InSuffix.size()
		&& InText.compare(InText.size() - InSuffix.size(), InSuffix.size(), InSuffix) == 0;
}

} // namespace

ConfigReader& ConfigReader::GetInstance()
{
	static ConfigReader Instance;
	return Instance;
}

const ObjectConfig& ConfigReader::GetObjectData(const std::string& InId) const
{
	static const ObjectConfig EMPTY;

	auto FindIt = ConfigsData.find(InId);
	if ( FindIt == ConfigsData.end() )
	{
		CCLOGWARN("ConfigReader: config not found, id - %s", InId.c_str());
		return EMPTY;
	}
	return FindIt->second;
}

const model::IConfigNode& ConfigReader::GetConfig(const std::string& InId) const
{
	static const JsonConfigNode Empty(model::JsonValue(), "<missing>");
	const ObjectConfig& Config = GetObjectData(InId);
	return Config.Node ? *Config.Node : Empty;
}

bool ConfigReader::HasConfig(const std::string& InId) const
{
	return ConfigsData.find(InId) != ConfigsData.end();
}

std::vector<std::string> ConfigReader::GetConfigIds(std::string_view InPrefix) const
{
	std::vector<std::string> Ids;
	for ( const auto& Config : ConfigsData )
	{
		if ( Config.first.compare(0, InPrefix.size(), InPrefix) == 0 )
		{
			Ids.push_back(Config.first);
		}
	}
	return Ids;
}

bool ConfigReader::LoadConfigFile(const std::string& InFullPath, const std::string& InId)
{
	auto FileUtilsInstance = cocos2d::FileUtils::getInstance();
	if ( !FileUtilsInstance->isFileExist(InFullPath) )
	{
		return false;
	}

	ObjectConfig Config = ParseConfig(FileUtilsInstance->getStringFromFile(InFullPath), InId);
	if ( Config.Values.empty() )
	{
		return false;
	}
	ConfigsData[InId] = std::move(Config);
	return true;
}

void ConfigReader::ReadObjectConfigs(const std::string& InFolderPath)
{
	auto FileUtilsInstance = cocos2d::FileUtils::getInstance();

	// The folder may live under the default resources root or under any search path.
	std::vector<std::string> Roots = FileUtilsInstance->getSearchPaths();
	Roots.insert(Roots.begin(), FileUtilsInstance->getDefaultResourceRootPath());

	for ( const auto& CurrentRoot : Roots )
	{
		std::string RootDir = NormalizeSlashes(CurrentRoot);
		if ( !RootDir.empty() && RootDir.back() != '/' )
		{
			RootDir += '/';
		}

		const std::string CurrentFolder = RootDir + InFolderPath;
		if ( !FileUtilsInstance->isDirectoryExist(CurrentFolder) )
		{
			continue;
		}

		std::vector<std::string> Files;
		FileUtilsInstance->listFilesRecursively(CurrentFolder, &Files);
		for ( const auto& File : Files )
		{
			const std::string Path = NormalizeSlashes(File);
			if ( !EndsWith(Path, ".json") || Path.compare(0, CurrentFolder.size(), CurrentFolder) != 0 )
			{
				continue;
			}

			const std::string CurrentId = InFolderPath + Path.substr(CurrentFolder.size());
			ConfigsData[CurrentId] = ParseConfig(FileUtilsInstance->getStringFromFile(Path), CurrentId);
		}
	}
}

} // namespace tanks

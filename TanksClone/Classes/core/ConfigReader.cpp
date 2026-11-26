#include "core/ConfigReader.h"

#include <algorithm>
#include <vector>

#include "cocos2d.h"

#include "core/JsonConfigNode.h"
#include "model/JsonValue.h"
#include "templates/TContainers.h"

namespace game
{

	namespace
	{

		ObjectConfig ParseConfig(const std::string& InJsonText, const std::string& InId)
		{
			ObjectConfig Result;
			bool bIsValidJson = false;
			Result.Values = model::JsonValue::Parse(InJsonText, &bIsValidJson);
			if (!bIsValidJson || !Result.Values.IsObject())
			{
				CCLOGERROR("ConfigReader: failed to parse %s", InId.c_str());
				return Result;
			}

			Result.Node = MakeHandle<JsonConfigNode>(Result.Values, InId);
			return Result;
		}

		std::string NormalizeSlashes(std::string InPath)
		{
			std::replace(InPath.begin(), InPath.end(), '\\', '/');
			return InPath;
		}

		bool EndsWith(std::string_view InText, std::string_view InSuffix)
		{
			return InText.size() >= InSuffix.size() && InText.compare(InText.size() - InSuffix.size(), InSuffix.size(), InSuffix) == 0;
		}

	} // namespace

	const ObjectConfig& ConfigReader::GetObjectData(const std::string_view InId) const
	{
		static const ObjectConfig EMPTY;

		const ObjectConfig* const Found = ConfigsData.Find(InId);
		if (!Found)
		{
			CCLOGWARN("ConfigReader: config not found, id - %.*s", static_cast<int>(InId.size()), InId.data());
			return EMPTY;
		}
		return *Found;
	}

	const model::IConfigNode& ConfigReader::GetConfig(const std::string_view InId) const
	{
		static const JsonConfigNode Empty(model::JsonValue(), "<missing>");
		const ObjectConfig& Config = GetObjectData(InId);
		return Config.Node ? *Config.Node : Empty;
	}

	bool ConfigReader::HasConfig(const std::string_view InId) const
	{
		return ConfigsData.Find(InId) != nullptr;
	}

	TArray<FPersistentString> ConfigReader::GetConfigIds(const std::string_view InPrefix) const
	{
		TArray<FPersistentString> Ids;
		for (const auto& Config : ConfigsData)
		{
			if (Config.Key.View().compare(0, InPrefix.size(), InPrefix) != 0)
			{
				continue;
			}
			Ids.PushBack(Config.Key);
		}
		return Ids;
	}

	bool ConfigReader::LoadConfigFile(const std::string& InFullPath, const std::string& InId)
	{
		auto FileUtilsInstance = cocos2d::FileUtils::getInstance();
		if (!FileUtilsInstance->isFileExist(InFullPath))
		{
			return false;
		}

		ObjectConfig Config = ParseConfig(FileUtilsInstance->getStringFromFile(InFullPath), InId);
		if (Config.Values.GetMembers().IsEmpty())
		{
			return false;
		}
		ConfigsData.InsertOrAssign(FPersistentString{ InId }, std::move(Config));
		return true;
	}

	bool ConfigReader::LoadConfigText(const std::string& InText, const std::string& InId)
	{
		ObjectConfig Config = ParseConfig(InText, InId);
		if (Config.Values.GetMembers().IsEmpty())
		{
			return false;
		}
		ConfigsData.InsertOrAssign(FPersistentString{ InId }, std::move(Config));
		return true;
	}

	void ConfigReader::ReadObjectConfigs(const std::string& InFolderPath)
	{
		auto FileUtilsInstance = cocos2d::FileUtils::getInstance();

		// The folder may live under the default resources root or under any search path.
		std::vector<std::string> Roots = FileUtilsInstance->getSearchPaths();
		Roots.insert(Roots.begin(), FileUtilsInstance->getDefaultResourceRootPath());

		for (const auto& CurrentRoot : Roots)
		{
			std::string RootDir = NormalizeSlashes(CurrentRoot);
			if (!RootDir.empty() && RootDir.back() != '/')
			{
				RootDir += '/';
			}

			const std::string CurrentFolder = RootDir + InFolderPath;
			if (!FileUtilsInstance->isDirectoryExist(CurrentFolder))
			{
				continue;
			}

			std::vector<std::string> Files;
			FileUtilsInstance->listFilesRecursively(CurrentFolder, &Files);
			for (const auto& File : Files)
			{
				const std::string Path = NormalizeSlashes(File);
				if (!EndsWith(Path, ".json") || Path.compare(0, CurrentFolder.size(), CurrentFolder) != 0)
				{
					continue;
				}

				const std::string CurrentId = InFolderPath + Path.substr(CurrentFolder.size());
				if (ConfigsData.Find(CurrentId))
				{
					continue;
				}
				ConfigsData.InsertOrAssign(FPersistentString{ CurrentId }, ParseConfig(FileUtilsInstance->getStringFromFile(Path), CurrentId));
			}
		}
	}

} // namespace game

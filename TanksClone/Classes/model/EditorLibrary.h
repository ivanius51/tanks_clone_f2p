#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/CustomLibrary.h"

namespace game::model
{
enum class LibrarySource
{
	Campaign,
	Custom,
	Test
};

/** One level in the editor's Library: where it comes from, its id and the name shown in the list. */
struct LibraryEntry
{
	LibrarySource Source = LibrarySource::Custom;
	std::string Id;
	std::string Title;
	std::string Path;
};

/** A level a config registry knows: its registry id and the file it lives in. */
struct LibraryOriginal
{
	std::string Id;
	std::string Path;
};

enum class SaveTarget
{
	SourceFile,
	CustomCopy
};

std::vector<LibraryEntry> BuildLibraryEntries(const std::vector<LibraryOriginal>& InCampaign, const std::vector<LibraryOriginal>& InTest,
	const CustomLibrary& InCustom);
std::vector<LibraryEntry> BuildCampaignEntries(const CampaignDef& InCampaign);
std::vector<LibraryEntry> FilterLibraryEntries(const std::vector<LibraryEntry>& InEntries, LibrarySource InSource);
std::string GetLevelFileTitle(std::string_view InPath);
SaveTarget ChooseSaveTarget(bool bInDev, bool bInOpenedOriginal);
std::string JoinSourcePath(std::string_view InRoot, std::string_view InPath);
} // namespace game::model

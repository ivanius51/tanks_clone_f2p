#pragma once

#include <string>
#include <string_view>

#include "model/CustomLibrary.h"
#include "templates/TContainers.h"

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

TArray<LibraryEntry> BuildLibraryEntries(const TArray<LibraryOriginal>& InCampaign, const TArray<LibraryOriginal>& InTest,
	const CustomLibrary& InCustom);
TArray<LibraryEntry> BuildCampaignEntries(const CampaignDef& InCampaign);
TArray<LibraryEntry> FilterLibraryEntries(const TArray<LibraryEntry>& InEntries, LibrarySource InSource);
std::string GetLevelFileTitle(std::string_view InPath);
SaveTarget ChooseSaveTarget(bool bInDev, bool bInOpenedOriginal);
std::string JoinSourcePath(std::string_view InRoot, std::string_view InPath);
} // namespace game::model

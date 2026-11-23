#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/CustomLibrary.h"

namespace tanks::model
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

// Campaign entries first, then the custom levels, then the test levels (only when bInDev).
std::vector<LibraryEntry> BuildLibraryEntries(const std::vector<LibraryOriginal>& InCampaign, const std::vector<LibraryOriginal>& InTest,
	const CustomLibrary& InCustom, bool bInDev);
// The entries of one source.
std::vector<LibraryEntry> FilterLibraryEntries(const std::vector<LibraryEntry>& InEntries, LibrarySource InSource);
// The file name without folders and extension ("configs/levels/campaign/level_3.json" -> "level_3").
std::string GetLevelFileTitle(std::string_view InPath);
// An original goes back into the source tree only in a build with debug tools; everything else saves a custom copy.
SaveTarget ChooseSaveTarget(bool bInDev, bool bInOpenedOriginal);
// InRoot and InPath joined with exactly one slash.
std::string JoinSourcePath(std::string_view InRoot, std::string_view InPath);

} // namespace tanks::model

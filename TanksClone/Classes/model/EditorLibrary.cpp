#include "model/EditorLibrary.h"

namespace tanks::model
{

std::string GetLevelFileTitle(const std::string_view InPath)
{
	const std::size_t Slash = InPath.find_last_of("/\\");
	std::string_view Name = Slash == std::string_view::npos ? InPath : InPath.substr(Slash + 1);
	const std::size_t Dot = Name.find_last_of('.');
	if ( Dot != std::string_view::npos )
	{
		Name = Name.substr(0, Dot);
	}
	return std::string(Name);
}

std::vector<LibraryEntry> BuildLibraryEntries(const std::vector<LibraryOriginal>& InCampaign, const std::vector<LibraryOriginal>& InTest,
	const CustomLibrary& InCustom, const bool bInDev)
{
	std::vector<LibraryEntry> Entries;
	for ( const LibraryOriginal& Original : InCampaign )
	{
		Entries.push_back(LibraryEntry{ LibrarySource::Campaign, Original.Id, GetLevelFileTitle(Original.Path), Original.Path });
	}
	for ( const std::string& Id : InCustom.GetLevelIds() )
	{
		Entries.push_back(LibraryEntry{ LibrarySource::Custom, Id, Id, std::string() });
	}
	if ( bInDev )
	{
		for ( const LibraryOriginal& Original : InTest )
		{
			Entries.push_back(LibraryEntry{ LibrarySource::Test, Original.Id, GetLevelFileTitle(Original.Path), Original.Path });
		}
	}
	return Entries;
}

std::vector<LibraryEntry> FilterLibraryEntries(const std::vector<LibraryEntry>& InEntries, const LibrarySource InSource)
{
	std::vector<LibraryEntry> Result;
	for ( const LibraryEntry& Entry : InEntries )
	{
		if ( Entry.Source == InSource )
		{
			Result.push_back(Entry);
		}
	}
	return Result;
}

SaveTarget ChooseSaveTarget(const bool bInDev, const bool bInOpenedOriginal)
{
	return bInDev && bInOpenedOriginal ? SaveTarget::SourceFile : SaveTarget::CustomCopy;
}

std::string JoinSourcePath(const std::string_view InRoot, const std::string_view InPath)
{
	std::string Root(InRoot);
	while ( !Root.empty() && (Root.back() == '/' || Root.back() == '\\') )
	{
		Root.pop_back();
	}
	std::string_view Path = InPath;
	while ( !Path.empty() && (Path.front() == '/' || Path.front() == '\\') )
	{
		Path.remove_prefix(1);
	}
	return Root.empty() ? std::string(Path) : Root + "/" + std::string(Path);
}

} // namespace tanks::model

#include "model/EditorLibrary.h"
#include "templates/TContainers.h"

namespace game::model
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

TArray<LibraryEntry> BuildLibraryEntries(const TArray<LibraryOriginal>& InCampaign, const TArray<LibraryOriginal>& InTest,
	const CustomLibrary& InCustom)
{
	TArray<LibraryEntry> Entries;
	for ( const LibraryOriginal& Original : InCampaign )
	{
		Entries.PushBack(LibraryEntry{ LibrarySource::Campaign, Original.Id, GetLevelFileTitle(Original.Path), Original.Path });
	}
	for ( const std::string& Id : InCustom.GetLevelIds() )
	{
		Entries.PushBack(LibraryEntry{ LibrarySource::Custom, Id, Id, std::string() });
	}
	for ( const LibraryOriginal& Original : InTest )
	{
		Entries.PushBack(LibraryEntry{ LibrarySource::Test, Original.Id, GetLevelFileTitle(Original.Path), Original.Path });
	}
	return Entries;
}

TArray<LibraryEntry> BuildCampaignEntries(const CampaignDef& InCampaign)
{
	TArray<LibraryEntry> Entries;
	for ( const std::string& Id : InCampaign.LevelIds )
	{
		Entries.PushBack(LibraryEntry{ LibrarySource::Custom, Id, Id, std::string() });
	}
	return Entries;
}

TArray<LibraryEntry> FilterLibraryEntries(const TArray<LibraryEntry>& InEntries, const LibrarySource InSource)
{
	TArray<LibraryEntry> Result;
	for ( const LibraryEntry& Entry : InEntries )
	{
		if ( Entry.Source == InSource )
		{
			Result.PushBack(Entry);
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

} // namespace game::model

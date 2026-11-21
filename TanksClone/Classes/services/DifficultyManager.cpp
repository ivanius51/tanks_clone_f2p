#include "services/DifficultyManager.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"

namespace tanks
{

void DifficultyManager::Init()
{
	const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::Difficulty, model::ResourceKind::Config);
	if ( const model::IConfigNode* const Node = GameContext::Get().GetManager<ConfigReader>().GetConfig(Path).FindChild("difficulty") )
	{
		Book.Load(*Node);
	}
}

const model::DifficultyDef& DifficultyManager::GetCurrent() const
{
	return Book.Find(GameContext::Get().GetManager<ProfileManager>().Get().Difficulty);
}

void DifficultyManager::SelectNext()
{
	std::string& Difficulty = GameContext::Get().GetManager<ProfileManager>().Get().Difficulty;
	Difficulty = Book.GetNextId(Difficulty);
}

const model::DifficultyBook& DifficultyManager::GetBook() const
{
	return Book;
}

} // namespace tanks

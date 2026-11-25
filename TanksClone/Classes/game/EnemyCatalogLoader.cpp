#include "game/EnemyCatalogLoader.h"

namespace game
{

EnemyDef EnemyCatalogLoader::ReadEnemy(const model::IConfigNode& InNode, const std::string_view InConfigId)
{
	EnemyDef Def;
	Def.ConfigId = std::string(InConfigId);
	Def.Id = IdFromConfigId(InConfigId);
	Def.NameKey = InNode.GetValueOr("name", Def.Id.c_str());
	Def.Atlas = InNode.GetValueOr("atlas", "");
	InNode.TryRead("enemy_type", Def.Kind);
	return Def;
}

std::string EnemyCatalogLoader::IdFromConfigId(const std::string_view InConfigId)
{
	const std::size_t SlashPos = InConfigId.find_last_of('/');
	const std::string_view FileName = SlashPos == std::string_view::npos ? InConfigId : InConfigId.substr(SlashPos + 1);
	const std::size_t DotPos = FileName.find_last_of('.');
	return std::string(DotPos == std::string_view::npos ? FileName : FileName.substr(0, DotPos));
}

} // namespace game

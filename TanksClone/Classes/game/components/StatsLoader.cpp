#include "game/components/StatsLoader.h"

#include "cocos2d.h"
#include "templates/TContainers.h"

namespace game
{

TArray<StatModifier> StatsLoader::ReadModifiers(const model::IConfigNode& InNode)
{
	TArray<StatModifier> Result;
	Result.Reserve(InNode.GetItemCount());
	for ( std::size_t Index = 0; Index < InNode.GetItemCount(); ++Index )
	{
		StatModifier Modifier;
		if ( ReadModifier(*InNode.GetItem(Index), Modifier) )
		{
			Result.PushBack(Modifier);
		}
		else
		{
			CCLOGERROR("Stats: unknown stat in a modifier");
		}
	}
	return Result;
}

bool StatsLoader::ReadModifier(const model::IConfigNode& InNode, StatModifier& OutModifier)
{
	StatModifier Modifier;
	if ( !InNode.TryRead("stat", Modifier.Stat) )
	{
		return false;
	}
	Modifier.Add = InNode.GetValueOr("add", 0.0f);
	Modifier.Mul = InNode.GetValueOr("mul", 1.0f);
	OutModifier = Modifier;
	return true;
}

} // namespace game

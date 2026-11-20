#include "game/components/StatsLoader.h"

#include "cocos2d.h"

namespace tanks
{

std::vector<StatModifier> StatsLoader::ReadModifiers(const model::IConfigNode& InNode)
{
	std::vector<StatModifier> Result;
	Result.reserve(InNode.GetItemCount());
	for ( std::size_t Index = 0; Index < InNode.GetItemCount(); ++Index )
	{
		StatModifier Modifier;
		if ( ReadModifier(*InNode.GetItem(Index), Modifier) )
		{
			Result.push_back(Modifier);
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

} // namespace tanks

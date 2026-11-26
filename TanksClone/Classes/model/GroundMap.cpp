#include "model/GroundMap.h"

#include <cmath>

namespace game::model
{
void GroundMap::Build(const TArray<std::string>& InGround, const LevelLegend& InLegend, const int InColumns, const int InRows)
{
	Clear();
	if ( InGround.IsEmpty() || InColumns <= 0 || InRows <= 0 )
	{
		return;
	}
	Columns = InColumns;
	Rows = InRows;
	bool bAnyEffect = false;
	for ( int Row = 0; Row < InRows; ++Row )
	{
		for ( int Column = 0; Column < InColumns; ++Column )
		{
			GroundEffect Effect;
			if ( Row < static_cast<int>(InGround.Size()) && Column < static_cast<int>(InGround[Row].size()) )
			{
				if ( const GroundEntry* const Entry = InLegend.FindGround(InGround[Row][Column]) )
				{
					Effect.SpeedScale = Entry->SpeedScale;
					Effect.bLeavesMarks = Entry->bLeavesMarks;
					Effect.bBlocksMovement = Entry->bBlocksMovement;
					Effect.PassBuff = Entry->PassBuff;
					Effect.ConcealSightTiles = Entry->ConcealSightTiles;
					Effect.ConcealRevealTime = Entry->ConcealRevealTime;
					Effect.ConcealOpacity = Entry->ConcealOpacity;
					bAnyEffect = true;
				}
			}
			Cells.PushBack(Effect);
		}
	}
	if ( !bAnyEffect )
	{
		Clear();
	}
}

void GroundMap::Clear()
{
	Columns = 0;
	Rows = 0;
	Cells.Clear();
}

GroundEffect GroundMap::GetEffect(const int InColumn, const int InRow) const
{
	if ( InColumn < 0 || InRow < 0 || InColumn >= Columns || InRow >= Rows )
	{
		return GroundEffect{};
	}
	return Cells[static_cast<std::size_t>(InRow) * Columns + InColumn];
}

GroundEffect GroundMap::GetEffectAtWorld(const float InX, const float InY, const float InTileSize) const
{
	if ( Cells.IsEmpty() || InTileSize <= 0.0f )
	{
		return GroundEffect{};
	}
	const int Column = static_cast<int>(std::floor(InX / InTileSize));
	const int Row = Rows - 1 - static_cast<int>(std::floor(InY / InTileSize));
	return GetEffect(Column, Row);
}
} // namespace game::model

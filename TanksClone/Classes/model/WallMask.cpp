#include "model/WallMask.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace tanks::model
{

namespace
{
	constexpr float ParallelEpsilon = 1e-6f;
	// A blast that only touches the edge of a line of cells leaves it alone.
	constexpr float LineEpsilon = 0.01f;

	bool SweepPointAgainstBox(const float InFromX, const float InFromY, const float InDeltaX, const float InDeltaY,
		const float InLeft, const float InBottom, const float InRight, const float InTop, float& OutFraction)
	{
		const std::array<float, 2> From = { InFromX, InFromY };
		const std::array<float, 2> Delta = { InDeltaX, InDeltaY };
		const std::array<float, 2> Low = { InLeft, InBottom };
		const std::array<float, 2> High = { InRight, InTop };

		float Enter = 0.0f;
		float Exit = 1.0f;
		for ( int Axis = 0; Axis < 2; ++Axis )
		{
			if ( std::fabs(Delta[Axis]) < ParallelEpsilon )
			{
				if ( From[Axis] <= Low[Axis] || From[Axis] >= High[Axis] )
				{
					return false;
				}
				continue;
			}
			float T1 = (Low[Axis] - From[Axis]) / Delta[Axis];
			float T2 = (High[Axis] - From[Axis]) / Delta[Axis];
			if ( T1 > T2 )
			{
				std::swap(T1, T2);
			}
			Enter = std::max(Enter, T1);
			Exit = std::min(Exit, T2);
			if ( Enter >= Exit )
			{
				return false;
			}
		}
		OutFraction = Enter;
		return true;
	}
}

WallMask::WallMask(const int InCellsPerSide, const int InCellHealth)
	: GridSize(std::max(1, InCellsPerSide))
	, FullHealth(std::max(1, InCellHealth))
	, Health(GridSize * GridSize, FullHealth)
{
}

int WallMask::GetCellsPerSide() const
{
	return GridSize;
}

int WallMask::Index(const int InColumn, const int InRow) const
{
	return InRow * GridSize + InColumn;
}

bool WallMask::IsSolid(const int InColumn, const int InRow) const
{
	return GetCellHealth(InColumn, InRow) > 0;
}

int WallMask::GetCellHealth(const int InColumn, const int InRow) const
{
	if ( InColumn < 0 || InRow < 0 || InColumn >= GridSize || InRow >= GridSize )
	{
		return 0;
	}
	return Health[Index(InColumn, InRow)];
}

int WallMask::GetFullCellHealth() const
{
	return FullHealth;
}

int WallMask::CountSolid() const
{
	return static_cast<int>(std::count_if(Health.begin(), Health.end(), [](const int InHealth) { return InHealth > 0; }));
}

bool WallMask::IsEmpty() const
{
	return CountSolid() == 0;
}

void WallMask::CellAt(const HitSide InSide, const int InLine, const int InStep, int& InColumn, int& InRow) const
{
	switch ( InSide )
	{
		case HitSide::Bottom: InColumn = InLine; InRow = InStep; break;
		case HitSide::Top: InColumn = InLine; InRow = GridSize - 1 - InStep; break;
		case HitSide::Left: InColumn = InStep; InRow = InLine; break;
		case HitSide::Right: InColumn = GridSize - 1 - InStep; InRow = InLine; break;
	}
}

int WallMask::Chip(const HitSide InSide, const float InFrom, const float InTo, const int InDepth, const int InDamage)
{
	int Removed = 0;
	for ( int Line = 0; Line < GridSize; ++Line )
	{
		if ( InFrom >= Line + 1 - LineEpsilon || InTo <= Line + LineEpsilon )
		{
			continue;
		}

		int HitCells = 0;
		for ( int CurrentStep = 0; CurrentStep < GridSize && HitCells < InDepth; ++CurrentStep )
		{
			int Column = 0;
			int Row = 0;
			CellAt(InSide, Line, CurrentStep, Column, Row);
			int& CurrentHealth = Health[Index(Column, Row)];
			if ( CurrentHealth <= 0 )
			{
				if ( HitCells > 0 )
				{
					break;
				}
				continue;
			}
			CurrentHealth = std::max(0, CurrentHealth - InDamage);
			if ( CurrentHealth == 0 )
			{
				++Removed;
			}
			++HitCells;
		}
	}
	return Removed;
}

bool WallMask::Overlaps(const float InLeft, const float InBottom, const float InRight, const float InTop) const
{
	const int FirstColumn = std::max(0, static_cast<int>(std::floor(InLeft)));
	const int LastColumn = std::min(GridSize - 1, static_cast<int>(std::ceil(InRight)) - 1);
	const int FirstRow = std::max(0, static_cast<int>(std::floor(InBottom)));
	const int LastRow = std::min(GridSize - 1, static_cast<int>(std::ceil(InTop)) - 1);
	for ( int Row = FirstRow; Row <= LastRow; ++Row )
	{
		for ( int Column = FirstColumn; Column <= LastColumn; ++Column )
		{
			if ( IsSolid(Column, Row) && InLeft < Column + 1 && InRight > Column && InBottom < Row + 1 && InTop > Row )
			{
				return true;
			}
		}
	}
	return false;
}

bool WallMask::Sweep(const float InFromX, const float InFromY, const float InToX, const float InToY, const float InHalf, float& OutFraction) const
{
	bool bFound = false;
	float Best = 1.0f;
	for ( int Row = 0; Row < GridSize; ++Row )
	{
		for ( int Column = 0; Column < GridSize; ++Column )
		{
			float T = 0.0f;
			if ( IsSolid(Column, Row)
				&& SweepPointAgainstBox(InFromX, InFromY, InToX - InFromX, InToY - InFromY,
					Column - InHalf, Row - InHalf, Column + 1 + InHalf, Row + 1 + InHalf, T)
				&& T <= Best )
			{
				Best = T;
				bFound = true;
			}
		}
	}
	OutFraction = Best;
	return bFound;
}

} // namespace tanks::model
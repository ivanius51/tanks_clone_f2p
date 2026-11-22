#include "model/GridPath.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

namespace tanks::model
{

GridPath::GridPath(const int InWidth, const int InHeight)
	: Width(std::max(0, InWidth))
	, Height(std::max(0, InHeight))
	, Cells(static_cast<size_t>(Width) * Height, CellPass::Open)
{
}

int GridPath::GetWidth() const
{
	return Width;
}

int GridPath::GetHeight() const
{
	return Height;
}

bool GridPath::Contains(const GridCell& InCell) const
{
	return InCell.X >= 0 && InCell.Y >= 0 && InCell.X < Width && InCell.Y < Height;
}

void GridPath::Set(const GridCell& InCell, const CellPass InPass)
{
	if ( Contains(InCell) )
	{
		Cells[Index(InCell)] = InPass;
	}
}

CellPass GridPath::Get(const GridCell& InCell) const
{
	return Contains(InCell) ? Cells[Index(InCell)] : CellPass::Blocked;
}

int GridPath::Index(const GridCell& InCell) const
{
	return InCell.Y * Width + InCell.X;
}

std::vector<GridCell> GridPath::Find(const GridCell& InFrom, const GridCell& InTo, const int InBreakCost) const
{
	if ( !Contains(InFrom) || !Contains(InTo) || InFrom == InTo || Get(InTo) == CellPass::Blocked )
	{
		return {};
	}
	constexpr int Unvisited = std::numeric_limits<int>::max();
	std::vector<int> Cost(Cells.size(), Unvisited);
	std::vector<int> CameFrom(Cells.size(), -1);
	using Entry = std::pair<int, int>;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> Open;
	Cost[Index(InFrom)] = 0;
	Open.push({ 0, Index(InFrom) });
	constexpr std::array<GridCell, 4> Steps = { GridCell{ 1, 0 }, GridCell{ -1, 0 }, GridCell{ 0, 1 }, GridCell{ 0, -1 } };
	const int Goal = Index(InTo);
	while ( !Open.empty() )
	{
		const auto [CurrentCost, Current] = Open.top();
		Open.pop();
		if ( Current == Goal )
		{
			break;
		}
		if ( CurrentCost > Cost[Current] )
		{
			continue;
		}
		const GridCell Here{ Current % Width, Current / Width };
		for ( const GridCell& Step : Steps )
		{
			const GridCell Next{ Here.X + Step.X, Here.Y + Step.Y };
			const CellPass Pass = Get(Next);
			if ( Pass == CellPass::Blocked )
			{
				continue;
			}
			const int NextCost = CurrentCost + (Pass == CellPass::Breakable ? std::max(1, InBreakCost) : 1);
			if ( NextCost < Cost[Index(Next)] )
			{
				Cost[Index(Next)] = NextCost;
				CameFrom[Index(Next)] = Current;
				Open.push({ NextCost, Index(Next) });
			}
		}
	}
	if ( Cost[Goal] == Unvisited )
	{
		return {};
	}
	std::vector<GridCell> Path;
	for ( int Current = Goal; Current != Index(InFrom); Current = CameFrom[Current] )
	{
		Path.push_back({ Current % Width, Current / Width });
	}
	std::reverse(Path.begin(), Path.end());
	return Path;
}

bool GridPath::IsReachable(const GridCell& InFrom, const GridCell& InTo) const
{
	return InFrom == InTo || !Find(InFrom, InTo, 1).empty();
}

} // namespace tanks::model

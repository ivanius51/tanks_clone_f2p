#include "model/TileMergeField.h"

#include <algorithm>
#include <cmath>

namespace game::model
{
namespace
{
constexpr float SettleDistance = 0.002f;
constexpr float SoftBelowThreshold = 0.03f;
constexpr float SearchMargin = 0.12f;

float Bilinear(const float InA, const float InB, const float InC, const float InD, const float InU, const float InV)
{
	return (InA * (1.0f - InU) + InB * InU) * (1.0f - InV) + (InC * (1.0f - InU) + InD * InU) * InV;
}
}

void TileMergeField::Resize(const int InColumns, const int InRows)
{
	Columns = std::max(InColumns, 0);
	Rows = std::max(InRows, 0);
	const size_t Count = static_cast<size_t>(Columns) * static_cast<size_t>(Rows);
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		Values[Material].Assign(Count, 0.0f);
		Targets[Material].Assign(Count, 0.0f);
	}
	Changed.Clear();
}

void TileMergeField::SetCell(const int InColumn, const int InRow, const MergeMaterial InMaterial)
{
	if ( !IsInside(InColumn, InRow) )
	{
		return;
	}
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		Targets[Material][Index(InColumn, InRow)] = Material == static_cast<size_t>(InMaterial) ? 1.0f : 0.0f;
	}
}

void TileMergeField::ClearCell(const int InColumn, const int InRow)
{
	if ( !IsInside(InColumn, InRow) )
	{
		return;
	}
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		Targets[Material][Index(InColumn, InRow)] = 0.0f;
	}
}

void TileMergeField::SetCellInstant(const int InColumn, const int InRow, const MergeMaterial InMaterial)
{
	SetCell(InColumn, InRow, InMaterial);
	if ( !IsInside(InColumn, InRow) )
	{
		return;
	}
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		Values[Material][Index(InColumn, InRow)] = Targets[Material][Index(InColumn, InRow)];
	}
}

bool TileMergeField::IsOccupied(const int InColumn, const int InRow) const
{
	if ( !IsInside(InColumn, InRow) )
	{
		return false;
	}
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		if ( Targets[Material][Index(InColumn, InRow)] > 0.5f )
		{
			return true;
		}
	}
	return false;
}

bool TileMergeField::Step(const float InDelta, const float InFadeTime)
{
	Changed.Clear();
	const float Rate = InFadeTime > 0.0f ? std::min(InDelta / InFadeTime, 1.0f) : 1.0f;
	const size_t CellCount = static_cast<size_t>(Columns) * static_cast<size_t>(Rows);
	for ( size_t Cell = 0; Cell < CellCount; ++Cell )
	{
		bool bMoved = false;
		for ( size_t Material = 0; Material < MaterialCount; ++Material )
		{
			const float Distance = Targets[Material][Cell] - Values[Material][Cell];
			if ( Distance == 0.0f )
			{
				continue;
			}
			bMoved = true;
			Values[Material][Cell] = std::fabs(Distance) <= SettleDistance ? Targets[Material][Cell] : Values[Material][Cell] + Distance * Rate;
		}
		if ( bMoved )
		{
			Changed.PushBack(static_cast<int>(Cell));
		}
	}
	return !Changed.IsEmpty();
}

float TileMergeField::Value(const size_t InMaterial, const int InColumn, const int InRow) const
{
	return IsInside(InColumn, InRow) ? Values[InMaterial][Index(InColumn, InRow)] : 0.0f;
}

float TileMergeField::SumValue(const int InColumn, const int InRow) const
{
	float Sum = 0.0f;
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		Sum += Value(Material, InColumn, InRow);
	}
	return std::min(Sum, 1.0f);
}

float TileMergeField::GetOccupancy(const MergeMaterial InMaterial, const float InX, const float InY) const
{
	const float U = InX - 0.5f;
	const float V = InY - 0.5f;
	const int Column = static_cast<int>(std::floor(U));
	const int Row = static_cast<int>(std::floor(V));
	const size_t Material = static_cast<size_t>(InMaterial);
	return Bilinear(Value(Material, Column, Row), Value(Material, Column + 1, Row), Value(Material, Column, Row + 1),
		Value(Material, Column + 1, Row + 1), U - static_cast<float>(Column), V - static_cast<float>(Row));
}

float TileMergeField::GetCoverage(const float InX, const float InY) const
{
	const float U = InX - 0.5f;
	const float V = InY - 0.5f;
	const int Column = static_cast<int>(std::floor(U));
	const int Row = static_cast<int>(std::floor(V));
	return Bilinear(SumValue(Column, Row), SumValue(Column + 1, Row), SumValue(Column, Row + 1), SumValue(Column + 1, Row + 1),
		U - static_cast<float>(Column), V - static_cast<float>(Row));
}

float TileMergeField::SquareCoverage(const float InX, const float InY) const
{
	const float U = InX - 0.5f;
	const float V = InY - 0.5f;
	const int Column = static_cast<int>(std::floor(U));
	const int Row = static_cast<int>(std::floor(V));
	const float FractionU = U - static_cast<float>(Column);
	const float FractionV = V - static_cast<float>(Row);
	const int NearRow = FractionV < 0.5f ? Row : Row + 1;
	const int NearColumn = FractionU < 0.5f ? Column : Column + 1;
	const float Horizontal = SumValue(Column, NearRow) * (1.0f - FractionU) + SumValue(Column + 1, NearRow) * FractionU;
	const float Vertical = SumValue(NearColumn, Row) * (1.0f - FractionV) + SumValue(NearColumn, Row + 1) * FractionV;
	return std::min(Horizontal, Vertical);
}

MergeSample TileMergeField::Sample(const float InX, const float InY, const float InJitterX, const float InJitterY,
	const TileMergeSettings& InSettings) const
{
	MergeSample Result;
	const float Threshold = InSettings.GetThreshold();
	const float Smooth = GetCoverage(InX, InY);
	const float Square = SquareCoverage(InX, InY);
	if ( std::max(Smooth, Square) < Threshold - SearchMargin )
	{
		return Result;
	}
	const int StepColumn = static_cast<int>(std::floor(InX + InJitterX));
	const int StepRow = static_cast<int>(std::floor(InY + InJitterY));
	float BestValue = -1.0f;
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		const MergeMaterial Kind = static_cast<MergeMaterial>(Material);
		const float Hard = InSettings.GetMaterial(Kind).bHardBorder ? InSettings.GetHardBorder() : 0.0f;
		const float Mixed = GetOccupancy(Kind, InX, InY) * (1.0f - Hard) + Value(Material, StepColumn, StepRow) * Hard;
		if ( Mixed > BestValue )
		{
			BestValue = Mixed;
			Result.Material = Kind;
		}
	}
	const float Sharp = InSettings.GetMaterial(Result.Material).CornerSharpness;
	Result.Coverage = Smooth * (1.0f - Sharp) + Square * Sharp;
	Result.bInside = Result.Coverage >= Threshold - SoftBelowThreshold;
	return Result;
}
} // namespace game::model

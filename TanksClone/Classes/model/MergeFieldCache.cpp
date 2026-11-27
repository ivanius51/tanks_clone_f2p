#include "model/MergeFieldCache.h"

#include <algorithm>
#include <cmath>

namespace game::model
{
namespace
{
constexpr float SoftBelowThreshold = 0.03f;
constexpr float SearchMargin = 0.12f;
constexpr float HardJitterScale = 3.0f;

float Rounded(const float InT, const float InRoundness)
{
	return InT + (InT * InT * (3.0f - 2.0f * InT) - InT) * InRoundness;
}

float Bilinear(const float InA, const float InB, const float InC, const float InD, const float InU, const float InV)
{
	return (InA * (1.0f - InU) + InB * InU) * (1.0f - InV) + (InC * (1.0f - InU) + InD * InU) * InV;
}
}

MergeFieldCache::MergeFieldCache(const TileMergeField& InField, const int InFirstColumn, const int InFirstRow, const int InEndColumn, const int InEndRow, const int InMargin)
	: Source(InField)
	, FirstColumn(InFirstColumn - InMargin)
	, FirstRow(InFirstRow - InMargin)
	, Columns(std::max(InEndColumn - InFirstColumn + 2 * InMargin, 0))
	, Rows(std::max(InEndRow - InFirstRow + 2 * InMargin, 0))
{
	const size_t Count = static_cast<size_t>(Columns) * static_cast<size_t>(Rows);
	Values.Assign(Count * MaterialCount, 0.0f);
	Sums.Assign(Count, 0.0f);
	for ( int Row = 0; Row < Rows; ++Row )
	{
		for ( int Column = 0; Column < Columns; ++Column )
		{
			const size_t Cell = static_cast<size_t>(Row) * static_cast<size_t>(Columns) + static_cast<size_t>(Column);
			float Sum = 0.0f;
			for ( size_t Material = 0; Material < MaterialCount; ++Material )
			{
				const float Amount = InField.GetValue(static_cast<MergeMaterial>(Material), FirstColumn + Column, FirstRow + Row);
				Values[Cell * MaterialCount + Material] = Amount;
				Sum += Amount;
			}
			Sums[Cell] = std::min(Sum, 1.0f);
		}
	}
}

float MergeFieldCache::Value(const size_t InMaterial, const int InColumn, const int InRow) const
{
	if ( !IsInWindow(InColumn, InRow) )
	{
		return Source.GetValue(static_cast<MergeMaterial>(InMaterial), InColumn, InRow);
	}
	return Values[Slot(InColumn, InRow) * MaterialCount + InMaterial];
}

float MergeFieldCache::SumValue(const int InColumn, const int InRow) const
{
	if ( IsInWindow(InColumn, InRow) )
	{
		return Sums[Slot(InColumn, InRow)];
	}
	float Sum = 0.0f;
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		Sum += Source.GetValue(static_cast<MergeMaterial>(Material), InColumn, InRow);
	}
	return std::min(Sum, 1.0f);
}

float MergeFieldCache::GetCoverage(const float InX, const float InY, const float InRoundness) const
{
	const float U = InX - 0.5f;
	const float V = InY - 0.5f;
	const int Column = static_cast<int>(std::floor(U));
	const int Row = static_cast<int>(std::floor(V));
	return Bilinear(SumValue(Column, Row), SumValue(Column + 1, Row), SumValue(Column, Row + 1), SumValue(Column + 1, Row + 1),
		Rounded(U - static_cast<float>(Column), InRoundness), Rounded(V - static_cast<float>(Row), InRoundness));
}

float MergeFieldCache::Occupancy(const size_t InMaterial, const float InX, const float InY) const
{
	const float U = InX - 0.5f;
	const float V = InY - 0.5f;
	const int Column = static_cast<int>(std::floor(U));
	const int Row = static_cast<int>(std::floor(V));
	return Bilinear(Value(InMaterial, Column, Row), Value(InMaterial, Column + 1, Row), Value(InMaterial, Column, Row + 1),
		Value(InMaterial, Column + 1, Row + 1), U - static_cast<float>(Column), V - static_cast<float>(Row));
}

float MergeFieldCache::SquareCoverage(const float InX, const float InY) const
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

bool MergeFieldCache::IsClearlyEmpty(const float InX, const float InY, const TileMergeSettings& InSettings) const
{
	const float Smooth = GetCoverage(InX, InY, InSettings.GetRoundness());
	const float Square = SquareCoverage(InX, InY);
	return std::max(Smooth, Square) < InSettings.GetThreshold() - SearchMargin;
}

MergeSample MergeFieldCache::Sample(const float InX, const float InY, const float InJitterX, const float InJitterY, const TileMergeSettings& InSettings) const
{
	MergeSample Result;
	const float Threshold = InSettings.GetThreshold();
	const float Smooth = GetCoverage(InX, InY, InSettings.GetRoundness());
	const float Square = SquareCoverage(InX, InY);
	if ( std::max(Smooth, Square) < Threshold - SearchMargin )
	{
		return Result;
	}
	float BestValue = -1.0f;
	for ( size_t Material = 0; Material < MaterialCount; ++Material )
	{
		const MergeMaterial Kind = static_cast<MergeMaterial>(Material);
		const float Hard = IsFloorMaterial(Kind) ? InSettings.GetFloorHardBorder() : (InSettings.GetMaterial(Kind).bHardBorder ? InSettings.GetHardBorder() : 0.0f);
		const float Reach = std::min(Hard * HardJitterScale, 1.0f);
		const int StepColumn = static_cast<int>(std::floor(InX + InJitterX * Reach));
		const int StepRow = static_cast<int>(std::floor(InY + InJitterY * Reach));
		const float Mixed = Occupancy(Material, InX, InY) * (1.0f - Hard) + Value(Material, StepColumn, StepRow) * Hard;
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

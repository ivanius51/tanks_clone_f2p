#pragma once

#include "model/TileMergeField.h"

namespace game::model
{
/**
 * A copy of a window of a TileMergeField laid out for fast sampling: the per-material values of a cell sit side by
 * side and the clamped sum of a cell is stored once, so a coverage sample is four reads instead of twenty-eight. It
 * answers like the field it was copied from (same arithmetic, same results) and falls back to the field outside the
 * window. Built once per bake; read-only afterwards, so any thread may use it.
 */
class MergeFieldCache
{
public:
	/** The cells [InFirstColumn, InEndColumn) x [InFirstRow, InEndRow) plus InMargin cells on every side. */
	MergeFieldCache(const TileMergeField& InField, int InFirstColumn, int InFirstRow, int InEndColumn, int InEndRow, int InMargin);

	static constexpr bool bCanSkipEmpty = true;

	float GetCoverage(float InX, float InY, float InRoundness) const;
	MergeSample Sample(float InX, float InY, float InJitterX, float InJitterY, const TileMergeSettings& InSettings) const;
	/** True when Sample would find nothing here, so the caller can skip the jitter noise. */
	bool IsClearlyEmpty(float InX, float InY, const TileMergeSettings& InSettings) const;

private:
	static constexpr size_t MaterialCount = static_cast<size_t>(MergeMaterialCount);

	bool IsInWindow(int InColumn, int InRow) const { return InColumn >= FirstColumn && InRow >= FirstRow && InColumn < FirstColumn + Columns && InRow < FirstRow + Rows; }
	size_t Slot(int InColumn, int InRow) const { return static_cast<size_t>(InRow - FirstRow) * static_cast<size_t>(Columns) + static_cast<size_t>(InColumn - FirstColumn); }
	float Value(size_t InMaterial, int InColumn, int InRow) const;
	float SumValue(int InColumn, int InRow) const;
	float Occupancy(size_t InMaterial, float InX, float InY) const;
	float SquareCoverage(float InX, float InY) const;

	const TileMergeField& Source;
	int FirstColumn = 0;
	int FirstRow = 0;
	int Columns = 0;
	int Rows = 0;
	TArray<float> Values;
	TArray<float> Sums;
};
} // namespace game::model

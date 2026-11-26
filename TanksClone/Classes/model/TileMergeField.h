#pragma once

#include "model/TileMergeSettings.h"
#include "templates/TContainers.h"

namespace game::model
{
/** What a point of the merged map is: inside the shape or not, which material, and how deep inside. */
struct MergeSample
{
	bool bInside = false;
	MergeMaterial Material = MergeMaterial::Brick;
	float Coverage = 0.0f;
};

/**
 * The merge field of a map: for every cell a 0-1 value per material. Values ease towards their target (a removed cell
 * fades out), are smoothed between cells and thresholded, so touching tiles melt into one shape with rounded outer
 * corners and flat straight edges. Positions are in cells, row 0 at the top. Look only: collision stays on whole cells.
 * Engine-free.
 */
class TileMergeField
{
public:
	void Resize(int InColumns, int InRows);
	int GetColumns() const { return Columns; }
	int GetRows() const { return Rows; }

	void SetCell(int InColumn, int InRow, MergeMaterial InMaterial, float InAmount = 1.0f);
	void ClearCell(int InColumn, int InRow);
	void SetCellInstant(int InColumn, int InRow, MergeMaterial InMaterial, float InAmount = 1.0f);
	bool IsOccupied(int InColumn, int InRow) const;

	bool Step(float InDelta, float InFadeTime);
	const TArray<int>& GetChangedCells() const { return Changed; }

	float GetOccupancy(MergeMaterial InMaterial, float InX, float InY) const;
	float GetCoverage(float InX, float InY, float InRoundness = 0.0f) const;
	MergeSample Sample(float InX, float InY, float InJitterX, float InJitterY, const TileMergeSettings& InSettings) const;

private:
	static constexpr size_t MaterialCount = static_cast<size_t>(MergeMaterialCount);

	size_t Index(int InColumn, int InRow) const { return static_cast<size_t>(InRow) * static_cast<size_t>(Columns) + static_cast<size_t>(InColumn); }
	bool IsInside(int InColumn, int InRow) const { return InColumn >= 0 && InRow >= 0 && InColumn < Columns && InRow < Rows; }
	float Value(size_t InMaterial, int InColumn, int InRow) const;
	float SumValue(int InColumn, int InRow) const;
	float SquareCoverage(float InX, float InY) const;
	void Activate(size_t InCell);

	int Columns = 0;
	int Rows = 0;
	TArray<float> Values[MaterialCount];
	TArray<float> Targets[MaterialCount];
	TArray<int> Changed;
	TArray<int> Active;
	TArray<std::uint8_t> IsActive;
};
} // namespace game::model

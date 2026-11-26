#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "model/ConfigNode.h"

namespace game::model
{
/** The kinds of tile that merge into one shape. */
enum class MergeMaterial
{
	Brick,
	Stone,
	Wood,
	Water,
	Forest,
	Dirt,
	Road
};

inline constexpr int MergeMaterialCount = 7;

inline constexpr bool IsFloorMaterial(const MergeMaterial InMaterial)
{
	return InMaterial == MergeMaterial::Water || InMaterial == MergeMaterial::Forest || InMaterial == MergeMaterial::Dirt || InMaterial == MergeMaterial::Road;
}

/** How one merge material shapes its edges. */
struct MergeMaterialSettings
{
	float CornerSharpness = 0.0f;
	bool bHardBorder = false;
};

/**
 * Look parameters of merged tiles (configs/balance/tile_merge.json): tile size inside its cell, corner and border
 * sharpness per material, overlay coverage and the easing time of a removed cell. Engine-free.
 */
class TileMergeSettings
{
public:
	void Load(const IConfigNode& InNode, std::string_view InTileset = {});

	float GetSize() const { return Size; }
	void SetSize(float InSize);
	float GetOverlayAmount() const { return OverlayAmount; }
	const std::array<std::uint8_t, 3>& GetOverlayColor() const { return OverlayColor; }
	int GetSubdivision() const { return Subdivision; }
	int GetBakeBudget() const { return BakeBudget; }
	void SetOverlayAmount(float InAmount);
	float GetFadeTime() const { return FadeTime; }
	float GetFloorHardBorder() const { return FloorHardBorder; }
	void SetFloorHardBorder(float InHardBorder);
	bool IsSideView() const { return SideFacePx > 0.0f; }
	float GetSideFacePx() const { return SideFacePx; }
	void SetSideFacePx(float InPixels);
	bool IsShadowEnabled() const { return ShadowOpacity > 0.0f; }
	float GetShadowOpacity() const { return ShadowOpacity; }
	void SetShadowOpacity(float InOpacity);
	float GetShadowOffsetPx() const { return ShadowOffsetPx; }
	void SetShadowOffsetPx(float InPixels);
	float GetShadowLengthPx() const { return ShadowLengthPx; }
	void SetShadowLengthPx(float InPixels);
	float GetEdgeLight() const { return EdgeLight; }
	void SetEdgeLight(float InLight);
	float GetTankShadowOpacity() const { return TankShadowOpacity; }
	float GetTankShadowRightPx() const { return TankShadowRightPx; }
	float GetTankShadowDownPx() const { return TankShadowDownPx; }
	float GetShadowSoft() const { return ShadowSoft; }
	float GetRoundness() const { return Roundness; }
	void SetRoundness(float InRoundness);
	void SetCornerSharpness(MergeMaterial InMaterial, float InSharpness);
	float GetHardBorder() const { return HardBorder; }
	void SetHardBorder(float InHardBorder);
	float GetPlaster() const { return Plaster; }
	void SetPlaster(float InPlaster);
	float GetCracks() const { return Cracks; }
	void SetCracks(float InCracks);
	const MergeMaterialSettings& GetMaterial(MergeMaterial InMaterial) const;
	bool IsEnabled() const { return bEnabled; }
	int GetCellPixels() const { return CellPixels; }
	int GetChunkCells() const { return ChunkCells; }
	float GetThreshold() const { return 1.0f - Size * 0.5f; }

	static std::string_view GetMaterialKey(MergeMaterial InMaterial);
	static bool FindMaterial(std::string_view InKey, MergeMaterial& OutMaterial);

private:
	void LoadOverlay(const IConfigNode& InNode, std::string_view InTileset);

	bool bEnabled = true;
	int Subdivision = 4;
	int BakeBudget = 4;
	std::array<std::uint8_t, 3> OverlayColor{ 236, 236, 240 };
	int CellPixels = 32;
	int ChunkCells = 8;
	float Size = 0.9f;
	float OverlayAmount = 0.3f;
	float FadeTime = 0.25f;
	float HardBorder = 0.3f;
	float FloorHardBorder = 0.0f;
	float SideFacePx = 24.0f;
	float ShadowOpacity = 0.5f;
	float ShadowOffsetPx = 12.0f;
	float ShadowLengthPx = 24.0f;
	float ShadowSoft = 0.15f;
	float EdgeLight = 0.3f;
	float TankShadowOpacity = 0.35f;
	float TankShadowRightPx = 6.0f;
	float TankShadowDownPx = 10.0f;
	float Roundness = 0.6f;
	float Plaster = 0.45f;
	float Cracks = 0.5f;
	std::array<MergeMaterialSettings, MergeMaterialCount> Materials{
		MergeMaterialSettings{ 1.0f, true }, MergeMaterialSettings{ 1.0f, true }, MergeMaterialSettings{ 1.0f, true },
		MergeMaterialSettings{ 0.0f, false }, MergeMaterialSettings{ 0.15f, false },
		MergeMaterialSettings{ 0.3f, false }, MergeMaterialSettings{ 1.0f, true } };
};
} // namespace game::model

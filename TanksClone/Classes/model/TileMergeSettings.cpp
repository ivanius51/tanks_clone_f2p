#include "model/TileMergeSettings.h"

#include <algorithm>

namespace game::model
{
namespace
{
constexpr float MinSize = 0.6f;
constexpr float MaxSize = 1.0f;
constexpr float MaxOverlay = 0.6f;
}

void TileMergeSettings::Load(const IConfigNode& InNode, const std::string_view InTileset)
{
	bEnabled = InNode.GetValueOr("enabled", bEnabled);
	CellPixels = std::clamp(InNode.GetValueOr("cell_pixels", CellPixels), 8, 128);
	ChunkCells = std::clamp(InNode.GetValueOr("chunk_cells", ChunkCells), 2, 32);
	SetSize(InNode.GetValueOr("size", Size));
	Subdivision = std::clamp(InNode.GetValueOr("wall_subdivision", Subdivision), 1, 8);
	BakeBudget = std::clamp(InNode.GetValueOr("bake_budget", BakeBudget), 1, 64);
	LoadOverlay(InNode, InTileset);
	FadeTime = InNode.GetValueOr("fade_time", FadeTime);
	SetHardBorder(InNode.GetValueOr("hard_border", HardBorder));
	SetRoundness(InNode.GetValueOr("roundness", Roundness));
	SetFloorHardBorder(InNode.GetValueOr("floor_hard_border", FloorHardBorder));
	SetSideFacePx(InNode.GetValueOr("side_face_px", SideFacePx));
	SetShadowOpacity(InNode.GetValueOr("shadow_opacity", ShadowOpacity));
	SetShadowOffsetPx(InNode.GetValueOr("shadow_offset_px", ShadowOffsetPx));
	SetShadowLengthPx(InNode.GetValueOr("shadow_length_px", ShadowLengthPx));
	ShadowSoft = std::clamp(InNode.GetValueOr("shadow_soft", ShadowSoft), 0.02f, 0.5f);
	Plaster = std::clamp(InNode.GetValueOr("plaster", Plaster), 0.0f, 1.0f);
	Cracks = std::clamp(InNode.GetValueOr("cracks", Cracks), 0.0f, 1.0f);
	for ( int Index = 0; Index < MergeMaterialCount; ++Index )
	{
		const IConfigNode* const Child = InNode.FindChild(GetMaterialKey(static_cast<MergeMaterial>(Index)));
		if ( !Child )
		{
			continue;
		}
		MergeMaterialSettings& Material = Materials[static_cast<size_t>(Index)];
		Material.CornerSharpness = std::clamp(Child->GetValueOr("corner_sharpness", Material.CornerSharpness), 0.0f, 1.0f);
		Material.bHardBorder = Child->GetValueOr("hard_border", Material.bHardBorder);
	}
}

void TileMergeSettings::LoadOverlay(const IConfigNode& InNode, const std::string_view InTileset)
{
	if ( InTileset.empty() )
	{
		return;
	}
	OverlayAmount = 0.0f;
	const IConfigNode* const Overlays = InNode.FindChild("overlays");
	const IConfigNode* const Overlay = Overlays ? Overlays->FindChild(InTileset) : nullptr;
	if ( !Overlay )
	{
		return;
	}
	SetOverlayAmount(Overlay->GetValueOr("amount", 0.0f));
	TArray<int> Color;
	Overlay->TryRead("color", Color);
	for ( size_t Channel = 0; Channel < OverlayColor.size() && Channel < Color.Size(); ++Channel )
	{
		OverlayColor[Channel] = static_cast<std::uint8_t>(std::clamp(Color[Channel], 0, 255));
	}
}

void TileMergeSettings::SetSize(const float InSize)
{
	Size = std::clamp(InSize, MinSize, MaxSize);
}

void TileMergeSettings::SetOverlayAmount(const float InAmount)
{
	OverlayAmount = std::clamp(InAmount, 0.0f, MaxOverlay);
}

void TileMergeSettings::SetFloorHardBorder(const float InHardBorder)
{
	FloorHardBorder = std::clamp(InHardBorder, 0.0f, 1.0f);
}

void TileMergeSettings::SetSideFacePx(const float InPixels)
{
	SideFacePx = std::clamp(InPixels, 0.0f, 64.0f);
}

void TileMergeSettings::SetShadowOpacity(const float InOpacity)
{
	ShadowOpacity = std::clamp(InOpacity, 0.0f, 1.0f);
}

void TileMergeSettings::SetShadowOffsetPx(const float InPixels)
{
	ShadowOffsetPx = std::clamp(InPixels, 0.0f, 40.0f);
}

void TileMergeSettings::SetShadowLengthPx(const float InPixels)
{
	ShadowLengthPx = std::clamp(InPixels, 0.0f, 64.0f);
}

void TileMergeSettings::SetRoundness(const float InRoundness)
{
	Roundness = std::clamp(InRoundness, 0.0f, 1.0f);
}

void TileMergeSettings::SetCornerSharpness(const MergeMaterial InMaterial, const float InSharpness)
{
	Materials[static_cast<size_t>(InMaterial)].CornerSharpness = std::clamp(InSharpness, 0.0f, 1.0f);
}

void TileMergeSettings::SetHardBorder(const float InHardBorder)
{
	HardBorder = std::clamp(InHardBorder, 0.0f, 1.0f);
}

void TileMergeSettings::SetPlaster(const float InPlaster)
{
	Plaster = std::clamp(InPlaster, 0.0f, 1.0f);
}

void TileMergeSettings::SetCracks(const float InCracks)
{
	Cracks = std::clamp(InCracks, 0.0f, 1.0f);
}

const MergeMaterialSettings& TileMergeSettings::GetMaterial(const MergeMaterial InMaterial) const
{
	return Materials[static_cast<size_t>(InMaterial)];
}

bool TileMergeSettings::FindMaterial(const std::string_view InKey, MergeMaterial& OutMaterial)
{
	for ( int Index = 0; Index < MergeMaterialCount; ++Index )
	{
		if ( GetMaterialKey(static_cast<MergeMaterial>(Index)) == InKey )
		{
			OutMaterial = static_cast<MergeMaterial>(Index);
			return true;
		}
	}
	return false;
}

std::string_view TileMergeSettings::GetMaterialKey(const MergeMaterial InMaterial)
{
	switch ( InMaterial )
	{
	case MergeMaterial::Brick: return "brick";
	case MergeMaterial::Stone: return "stone";
	case MergeMaterial::Wood: return "wood";
	case MergeMaterial::Water: return "water";
	case MergeMaterial::Forest: return "forest";
	case MergeMaterial::Dirt: return "dirt";
	case MergeMaterial::Road: return "road";
	}
	return "brick";
}
} // namespace game::model

#include "model/TileMergeSettings.h"

#include <algorithm>

namespace game::model
{
namespace
{
constexpr float MinSize = 0.6f;
constexpr float MaxSize = 1.0f;
constexpr float MaxOverlay = 0.6f;
constexpr std::uint32_t SizeHashMultiplier = 2654435761u;
constexpr float SizeHashScale = 16777216.0f;
}

void TileMergeSettings::Load(const IConfigNode& InNode, const std::string_view InTileset)
{
	bEnabled = InNode.GetValueOr("enabled", bEnabled);
	CellPixels = std::clamp(InNode.GetValueOr("cell_pixels", CellPixels), 8, 128);
	ChunkCells = std::clamp(InNode.GetValueOr("chunk_cells", ChunkCells), 2, 32);
	SetSize(InNode.GetValueOr("size", Size));
	Subdivision = std::clamp(InNode.GetValueOr("wall_subdivision", Subdivision), 1, 8);
	BakeBudget = std::clamp(InNode.GetValueOr("bake_budget", BakeBudget), 1, 64);
	PickBudgetMs = InNode.GetValueOr("pick_budget_ms", PickBudgetMs);
	LoadBakeBackend(InNode);
	LoadOverlay(InNode, InTileset);
	LoadSize(InNode, InTileset);
	FadeTime = InNode.GetValueOr("fade_time", FadeTime);
	SetHardBorder(InNode.GetValueOr("hard_border", HardBorder));
	SetRoundness(InNode.GetValueOr("roundness", Roundness));
	SetFloorHardBorder(InNode.GetValueOr("floor_hard_border", FloorHardBorder));
	SetSideFacePx(InNode.GetValueOr("side_face_px", SideFacePx));
	LoadGlobalShadow(InNode);
	SetEdgeLight(InNode.GetValueOr("edge_light", EdgeLight));
	LoadTankShadow(InNode);
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

void TileMergeSettings::ApplyQuality(const IConfigNode& InNode, const GraphicsQuality InQuality)
{
	const IConfigNode* const Levels = InNode.FindChild("quality");
	const IConfigNode* const Level = Levels ? Levels->FindChild(std::string(GetGraphicsQualityKey(InQuality))) : nullptr;
	if ( !Level )
	{
		return;
	}
	bEnabled = Level->GetValueOr("enabled", bEnabled);
	CellPixels = std::clamp(Level->GetValueOr("cell_pixels", CellPixels), 8, 128);
	bMipmaps = Level->GetValueOr("mipmaps", false);
	PatternScale = std::clamp(Level->GetValueOr("pattern_scale", 1.0f), 1.0f, 4.0f);
	SetSideFacePx(Level->GetValueOr("side_face_px", SideFacePx));
	LoadBakeBackend(*Level);
}

void TileMergeSettings::LoadBakeBackend(const IConfigNode& InNode)
{
	const std::string Name = InNode.GetValueOr("bake_backend", "auto");
	if ( Name == "reference" )
	{
		BakeBackend = MergeBakeBackend::Reference;
	}
	else if ( Name == "cached" )
	{
		BakeBackend = MergeBakeBackend::Cached;
	}
	else if ( Name == "gpu" )
	{
		BakeBackend = MergeBakeBackend::Gpu;
	}
	else if ( Name == "auto" )
	{
		BakeBackend = MergeBakeBackend::Auto;
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

void TileMergeSettings::LoadGlobalShadow(const IConfigNode& InNode)
{
	const IConfigNode* const Shadow = InNode.FindChild("global_shadow");
	if ( !Shadow )
	{
		return;
	}
	SetShadowOpacity(Shadow->GetValueOr("opacity", ShadowOpacity));
	SetShadowOffsetPx(Shadow->GetValueOr("offset_px", ShadowOffsetPx));
	SetShadowLengthPx(Shadow->GetValueOr("length_px", ShadowLengthPx));
	ShadowSoft = std::clamp(Shadow->GetValueOr("soft", ShadowSoft), 0.02f, 0.5f);
}

void TileMergeSettings::LoadTankShadow(const IConfigNode& InNode)
{
	const IConfigNode* const Shadow = InNode.FindChild("tank_shadow");
	if ( !Shadow )
	{
		return;
	}
	TankShadowOpacity = std::clamp(Shadow->GetValueOr("opacity", TankShadowOpacity), 0.0f, 1.0f);
	TankShadowRightPx = std::clamp(Shadow->GetValueOr("right_px", TankShadowRightPx), 0.0f, 40.0f);
	TankShadowDownPx = std::clamp(Shadow->GetValueOr("down_px", TankShadowDownPx), 0.0f, 40.0f);
}

void TileMergeSettings::LoadSize(const IConfigNode& InNode, const std::string_view InTileset)
{
	bHasSizeRange = false;
	if ( InTileset.empty() )
	{
		return;
	}
	const IConfigNode* const Sizes = InNode.FindChild("sizes");
	const IConfigNode* const Range = Sizes ? Sizes->FindChild(InTileset) : nullptr;
	if ( !Range )
	{
		return;
	}
	SetSizeRange(Range->GetValueOr("min", Size), Range->GetValueOr("max", Size));
}

void TileMergeSettings::SetSize(const float InSize)
{
	Size = std::clamp(InSize, MinSize, MaxSize);
	bHasSizeRange = false;
}

void TileMergeSettings::SetSizeRange(const float InMin, const float InMax)
{
	SizeMin = std::clamp(std::min(InMin, InMax), MinSize, MaxSize);
	SizeMax = std::clamp(std::max(InMin, InMax), MinSize, MaxSize);
	bHasSizeRange = true;
}

float TileMergeSettings::ResolveSize(const std::uint32_t InSeed) const
{
	if ( !bHasSizeRange )
	{
		return Size;
	}
	const std::uint32_t Mixed = InSeed * SizeHashMultiplier;
	const float Fraction = static_cast<float>((Mixed ^ (Mixed >> 15)) >> 8) / SizeHashScale;
	return SizeMin + (SizeMax - SizeMin) * Fraction;
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

void TileMergeSettings::SetEdgeLight(const float InLight)
{
	EdgeLight = std::clamp(InLight, 0.0f, 1.0f);
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

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

void TileMergeSettings::Load(const IConfigNode& InNode)
{
	SetSize(InNode.GetValueOr("size", Size));
	SetOverlayAmount(InNode.GetValueOr("overlay_amount", OverlayAmount));
	FadeTime = InNode.GetValueOr("fade_time", FadeTime);
	SetHardBorder(InNode.GetValueOr("hard_border", HardBorder));
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

void TileMergeSettings::SetSize(const float InSize)
{
	Size = std::clamp(InSize, MinSize, MaxSize);
}

void TileMergeSettings::SetOverlayAmount(const float InAmount)
{
	OverlayAmount = std::clamp(InAmount, 0.0f, MaxOverlay);
}

void TileMergeSettings::SetHardBorder(const float InHardBorder)
{
	HardBorder = std::clamp(InHardBorder, 0.0f, 1.0f);
}

const MergeMaterialSettings& TileMergeSettings::GetMaterial(const MergeMaterial InMaterial) const
{
	return Materials[static_cast<size_t>(InMaterial)];
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
	}
	return "brick";
}
} // namespace game::model

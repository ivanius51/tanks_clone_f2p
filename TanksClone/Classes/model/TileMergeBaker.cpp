#include "model/TileMergeBaker.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "model/MergeFieldCache.h"

namespace game::model
{
namespace
{
constexpr float TextureUnits = 64.0f;
constexpr float EdgeBand = 0.35f;
constexpr float SoftEdge = 0.03f;
constexpr float JitterRange = 0.9f;
constexpr float CrownSize = 0.2f;
constexpr int FaceSearchSteps = 5;
constexpr float LightLineEnd = 0.03f;
constexpr float LightLineShade = 1.25f;
constexpr float CreaseEnd = 0.07f;
constexpr float CreaseShade = 0.4f;
constexpr float FaceTopShade = 0.85f;
constexpr float FaceMidShade = 0.35f;
constexpr float FaceMidEnd = 0.77f;
constexpr float FaceFootTopShade = 0.2f;
constexpr float FaceFootBottomShade = 0.16f;
constexpr int ShadowTaps = 6;
constexpr float EdgeLightPixels = 2.0f;
constexpr float JitterFrequency = 3.1f;
constexpr float RimShade = 0.18f;
constexpr float RimDepth = 0.1f;
constexpr float PlasterEdgeWeight = 0.5f;
constexpr float PlasterNoiseWeight = 0.62f;
constexpr float PlasterReach = 0.55f;
constexpr float CrackWidth = 0.012f;
constexpr float RustReach = 0.22f;
constexpr float PlankLength = 56.0f;
constexpr float PlankHeight = 16.0f;
constexpr float PlankRadius = 5.0f;
constexpr float BrickWidth = 32.0f;
constexpr float BrickHeight = 16.0f;
constexpr float SlabWidth = 64.0f;
constexpr float SlabHeight = 32.0f;

float Smooth(const float InT)
{
	return InT * InT * (3.0f - 2.0f * InT);
}

float Clamp01(const float InValue)
{
	return std::clamp(InValue, 0.0f, 1.0f);
}

int FloorToInt(const float InValue)
{
	return static_cast<int>(std::floor(InValue));
}
}

TileMergeBaker::TileMergeBaker(const TileMergeSettings& InSettings, const std::uint32_t InSeed, const int InSubdivision)
	: Settings(InSettings)
	, Seed(InSeed)
	, CoverageThreshold(InSettings.ResolveThreshold(InSeed))
	, Subdivision(std::max(InSubdivision, 1))
{
}

void TileMergeBaker::SetSeed(const std::uint32_t InSeed)
{
	Seed = InSeed;
	CoverageThreshold = Settings.ResolveThreshold(InSeed);
}

float TileMergeBaker::Hash(const int InX, const int InY) const
{
	std::uint32_t Value = static_cast<std::uint32_t>(InX) * 374761393u + static_cast<std::uint32_t>(InY) * 668265263u + Seed * 2246822519u;
	Value = (Value ^ (Value >> 13)) * 1274126177u;
	Value ^= Value >> 16;
	return static_cast<float>(Value) / 4294967296.0f;
}

float TileMergeBaker::Noise(const float InX, const float InY) const
{
	const int X = FloorToInt(InX);
	const int Y = FloorToInt(InY);
	const float FractionX = Smooth(InX - static_cast<float>(X));
	const float FractionY = Smooth(InY - static_cast<float>(Y));
	const float Top = Hash(X, Y) * (1.0f - FractionX) + Hash(X + 1, Y) * FractionX;
	const float Bottom = Hash(X, Y + 1) * (1.0f - FractionX) + Hash(X + 1, Y + 1) * FractionX;
	return Top * (1.0f - FractionY) + Bottom * FractionY;
}

TileMergeBaker::Rgb TileMergeBaker::Brick(const float InU, const float InV, const float InX, const float InY, const float InEdge) const
{
	const int Row = FloorToInt(InV / BrickHeight);
	const float InRow = InV - static_cast<float>(Row) * BrickHeight;
	const float Offset = static_cast<float>(Row & 1) * BrickWidth * 0.5f;
	const int Column = FloorToInt((InU + Offset) / BrickWidth);
	const float InColumn = InU + Offset - static_cast<float>(Column) * BrickWidth;
	const int PixelU = FloorToInt(InU);
	const int PixelV = FloorToInt(InV);
	Rgb Color;
	if ( InRow < 2.0f || InColumn < 2.0f )
	{
		const float Grain = (Hash(PixelU, PixelV) - 0.5f) * 14.0f;
		Color = { 178.0f + Grain, 170.0f + Grain, 154.0f + Grain };
	}
	else
	{
		const float Tone = 0.82f + Hash(Column, Row) * 0.34f;
		const float Grain = (Hash(PixelU, PixelV) - 0.5f) * 18.0f + (Noise(InU * 0.15f, InV * 0.15f) - 0.5f) * 16.0f;
		const float Light = InRow < 4.0f ? 1.12f : 1.0f;
		Color = { (150.0f * Tone + Grain) * Light, (72.0f * Tone + Grain * 0.5f) * Light, (50.0f * Tone + Grain * 0.4f) * Light };
	}
	const float Plaster = Settings.GetPlaster();
	const float Patch = Noise(InX * 1.5f + 5.0f, InY * 1.5f + 2.0f) * PlasterNoiseWeight + InEdge * PlasterEdgeWeight + (Noise(InX * 8.0f, InY * 8.0f) - 0.5f) * 0.12f;
	const float Limit = 1.0f - Plaster * PlasterReach;
	if ( Patch <= Limit )
	{
		return Color;
	}
	const float Speck = (Noise(InX * 14.0f, InY * 14.0f) - 0.5f) * 26.0f;
	const float Rim = Patch - Limit < 0.025f ? 0.72f : 1.0f;
	return { (206.0f + Speck) * Rim, (190.0f + Speck) * Rim, (168.0f + Speck) * Rim };
}

TileMergeBaker::Rgb TileMergeBaker::Stone(const float InU, const float InV, const float InX, const float InY) const
{
	const int Row = FloorToInt(InV / SlabHeight);
	const float Offset = static_cast<float>(Row & 1) * SlabWidth * 0.5f;
	const int Column = FloorToInt((InU + Offset) / SlabWidth);
	const float InColumn = InU + Offset - static_cast<float>(Column) * SlabWidth;
	const float InRow = InV - static_cast<float>(Row) * SlabHeight;
	if ( InRow < 1.6f || InColumn < 1.6f )
	{
		return { 78.0f, 82.0f, 92.0f };
	}
	const float Tone = 0.9f + Hash(Column, Row) * 0.2f;
	const float Grain = (Noise(InU * 0.04f, InV * 0.04f) - 0.5f) * 30.0f + (Hash(FloorToInt(InU), FloorToInt(InV)) - 0.5f) * 10.0f;
	const float Light = InRow < 4.0f ? 1.08f : 1.0f;
	Rgb Color{ (118.0f * Tone + Grain) * Light, (124.0f * Tone + Grain) * Light, (136.0f * Tone + Grain) * Light };
	const float Cracks = Settings.GetCracks();
	const float Line = std::fabs(Noise(InU * 0.05f + Hash(Column, Row) * 40.0f, InV * 0.05f + Hash(Row, Column) * 40.0f) - 0.5f);
	const float Width = CrackWidth * Cracks * 2.0f;
	if ( Line < Width )
	{
		Color = { 58.0f, 62.0f, 72.0f };
	}
	else if ( Line < Width * 1.7f )
	{
		Color = { Color.R * 0.8f, Color.G * 0.8f, Color.B * 0.82f };
	}
	if ( Line < Width * 3.0f && Noise(InX * TextureUnits * 0.28f, InY * TextureUnits * 0.28f) > 1.0f - RustReach * Cracks )
	{
		Color = { 164.0f, 88.0f, 44.0f };
	}
	return Color;
}

TileMergeBaker::Rgb TileMergeBaker::Wood(const float InU, const float InV) const
{
	const int Row = FloorToInt(InV / PlankHeight);
	const float InRow = InV - static_cast<float>(Row) * PlankHeight;
	const float Offset = Hash(Row, 3) * PlankLength;
	const int Index = FloorToInt((InU + Offset) / PlankLength);
	const float InPlank = InU + Offset - static_cast<float>(Index) * PlankLength;
	const float GapStart = 1.0f + Hash(Index, Row) * 3.0f;
	const float GapEnd = 1.0f + Hash(Row, Index + 9) * 3.0f;
	const float CenterX = std::clamp(InPlank, GapStart + PlankRadius, PlankLength - GapEnd - PlankRadius);
	const float CenterY = std::clamp(InRow, 1.0f + PlankRadius, PlankHeight - 1.0f - PlankRadius);
	if ( std::hypot(InPlank - CenterX, InRow - CenterY) > PlankRadius )
	{
		return { 82.0f, 48.0f, 24.0f };
	}
	const float Tone = 0.8f + Hash(Index, Row + 50) * 0.4f;
	const float Grain = std::sin(InRow * 0.95f + Noise(InU * 0.03f + static_cast<float>(Row) * 7.0f, InV * 0.2f) * 7.0f) * 0.5f + 0.5f;
	float Shade = Tone * (0.9f + 0.12f * Grain);
	if ( InRow < 3.5f )
	{
		Shade *= 1.14f;
	}
	if ( InRow > 12.5f )
	{
		Shade *= 0.86f;
	}
	const float KnotChance = Hash(Index, Row + 90);
	const float KnotDistance = std::hypot((InPlank - (20.0f + Hash(Index, Row + 7) * 20.0f)) * 0.5f, InRow - 8.0f);
	if ( KnotChance > 0.7f && KnotDistance < 3.0f )
	{
		Shade *= 0.62f;
	}
	else if ( KnotChance > 0.7f && KnotDistance < 4.6f )
	{
		Shade *= 0.8f;
	}
	return { 172.0f * Shade, 104.0f * Shade, 50.0f * Shade };
}

TileMergeBaker::Rgb TileMergeBaker::Water(const float InX, const float InY) const
{
	const float Depth = Noise(InX * 2.2f, InY * 2.2f);
	const float WarpX = (Noise(InX * 6.0f + 3.0f, InY * 6.0f) - 0.5f) * 0.5f;
	const float WarpY = (Noise(InX * 6.0f, InY * 6.0f + 7.0f) - 0.5f) * 0.5f;
	const float RidgeA = 1.0f - std::fabs(Noise(InX * 9.0f + WarpX, InY * 9.0f + WarpY) * 2.0f - 1.0f);
	const float RidgeB = 1.0f - std::fabs(Noise(InX * 15.0f - WarpY, InY * 15.0f + WarpX) * 2.0f - 1.0f);
	const float Glint = std::pow(RidgeA, 9.0f) * 0.9f + std::pow(RidgeB, 12.0f) * 0.5f;
	const float Base = 0.75f + Depth * 0.5f;
	return { 22.0f * Base + Glint * 200.0f, 98.0f * Base + Glint * 190.0f, 168.0f * Base + Glint * 70.0f };
}

TileMergeBaker::Rgb TileMergeBaker::Forest(const float InX, const float InY) const
{
	const float Scaled = 1.0f / CrownSize;
	const float PointX = InX * Scaled;
	const float PointY = InY * Scaled;
	const int CellX = FloorToInt(PointX);
	const int CellY = FloorToInt(PointY);
	float Nearest = 9.0f;
	float Second = 9.0f;
	float Tone = 0.0f;
	for ( int OffsetY = -1; OffsetY <= 1; ++OffsetY )
	{
		for ( int OffsetX = -1; OffsetX <= 1; ++OffsetX )
		{
			const int NeighbourX = CellX + OffsetX;
			const int NeighbourY = CellY + OffsetY;
			const float CenterX = static_cast<float>(NeighbourX) + 0.2f + Hash(NeighbourX, NeighbourY + 31) * 0.6f;
			const float CenterY = static_cast<float>(NeighbourY) + 0.2f + Hash(NeighbourX + 57, NeighbourY) * 0.6f;
			const float Distance = std::hypot(PointX - CenterX, PointY - CenterY);
			if ( Distance < Nearest )
			{
				Second = Nearest;
				Nearest = Distance;
				Tone = Hash(NeighbourX + 11, NeighbourY + 5);
			}
			else if ( Distance < Second )
			{
				Second = Distance;
			}
		}
	}
	const float Dome = Clamp01(1.0f - Nearest * 1.15f);
	const float Gap = Clamp01((Second - Nearest) * 2.5f);
	const float Light = (0.55f + 0.7f * Dome) * (0.45f + 0.55f * Gap) * (0.8f + 0.4f * Tone);
	const float Leaf = (Noise(InX * 60.0f, InY * 60.0f) - 0.5f) * 0.18f;
	return { (40.0f + 40.0f * Tone) * (Light + Leaf), (112.0f + 50.0f * Tone) * (Light + Leaf), (38.0f + 22.0f * Tone) * (Light + Leaf) };
}

TileMergeBaker::Rgb TileMergeBaker::Dirt(const float InX, const float InY) const
{
	const float Large = Noise(InX * 3.0f, InY * 3.0f);
	const float Small = Noise(InX * 18.0f, InY * 18.0f);
	const float Grit = Hash(FloorToInt(InX * TextureUnits), FloorToInt(InY * TextureUnits));
	const float Streak = Noise(InX * 5.0f + Noise(InX * 11.0f, InY * 11.0f) * 2.0f, InY * 22.0f);
	float Shade = 0.72f + Large * 0.4f + (Small - 0.5f) * 0.28f + (Grit - 0.5f) * 0.12f;
	Rgb Color{ 150.0f * Shade, 86.0f * Shade, 62.0f * Shade };
	if ( Streak > 0.7f )
	{
		Color = { Color.R * 1.18f, Color.G * 1.14f, Color.B * 1.1f };
	}
	const float Puddle = Noise(InX * 4.0f + 40.0f, InY * 4.0f + 40.0f);
	if ( Puddle > 0.68f )
	{
		const float Wet = Clamp01((Puddle - 0.68f) * 6.0f) * 0.35f;
		Color = { Color.R + (176.0f - Color.R) * Wet, Color.G + (118.0f - Color.G) * Wet, Color.B + (104.0f - Color.B) * Wet };
	}
	const float Blade = Noise(InX * 30.0f + 90.0f, InY * 30.0f);
	if ( Blade > 0.84f && Large < 0.42f )
	{
		Color = { 110.0f, 160.0f, 50.0f };
	}
	return Color;
}

TileMergeBaker::Rgb TileMergeBaker::Road(const float InX, const float InY, const float InEdge) const
{
	const float Grain = (Hash(FloorToInt(InX * TextureUnits), FloorToInt(InY * TextureUnits)) - 0.5f) * 26.0f;
	const float Patch = (Noise(InX * 4.0f, InY * 4.0f) - 0.5f) * 16.0f;
	const float Base = 62.0f + Grain + Patch;
	if ( InEdge > 0.8f && InEdge < 0.9f && Noise(InX * 25.0f, InY * 25.0f) > 0.12f )
	{
		return { 214.0f + Grain, 214.0f + Grain, 208.0f + Grain };
	}
	return { Base, Base + 1.0f, Base + 2.0f };
}

TileMergeBaker::Rgb TileMergeBaker::Fill(const MergeMaterial InMaterial, const float InX, const float InY, const float InEdge) const
{
	const float U = InX * TextureUnits;
	const float V = InY * TextureUnits;
	switch ( InMaterial )
	{
	case MergeMaterial::Brick: return Brick(U, V, InX, InY, InEdge);
	case MergeMaterial::Stone: return Stone(U, V, InX, InY);
	case MergeMaterial::Wood: return Wood(U, V);
	case MergeMaterial::Water: return Water(InX, InY);
	case MergeMaterial::Forest: return Forest(InX, InY);
	case MergeMaterial::Dirt: return Dirt(InX, InY);
	case MergeMaterial::Road: return Road(InX, InY, InEdge);
	}
	return {};
}

void TileMergeBaker::AddOverlay(Rgb& InOut, const float InX, const float InY, const int InPixelX, const int InPixelY, const float InEdge) const
{
	const float Amount = Settings.GetOverlayAmount();
	if ( Amount <= 0.0f )
	{
		return;
	}
	const float Cluster = Noise(InX * 1.3f + 3.0f, InY * 1.3f + 9.0f);
	const float Chance = Amount * (0.35f + 1.3f * Cluster) + InEdge * 0.05f;
	if ( Hash(InPixelX / 2, InPixelY / 2) >= Chance )
	{
		return;
	}
	const float Strength = 0.3f + 0.25f * Hash(InPixelX, InPixelY);
	const std::array<std::uint8_t, 3>& Color = Settings.GetOverlayColor();
	InOut.R += (static_cast<float>(Color[0]) - InOut.R) * Strength;
	InOut.G += (static_cast<float>(Color[1]) - InOut.G) * Strength;
	InOut.B += (static_cast<float>(Color[2]) - InOut.B) * Strength;
}

void TileMergeBaker::Bake(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	BakeWith(InField, InChunk, OutRgba);
}

void TileMergeBaker::BakeShadow(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	BakeShadowWith(InField, InChunk, OutRgba);
}

int TileMergeBaker::ComputeReachCells() const
{
	const float ReachPixels = std::max({ Settings.GetSideFacePx(), EdgeLightPixels, Settings.GetShadowOffsetPx(), Settings.GetShadowLengthPx() });
	return static_cast<int>(std::ceil(ReachPixels / TextureUnits * static_cast<float>(Subdivision))) + 3;
}

template <typename TSampler>
void TileMergeBaker::BakeWith(const TSampler& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	const float Threshold = CoverageThreshold;
	const float CellPixels = static_cast<float>(InChunk.CellPixels);
	const int Width = InChunk.GetWidth();
	const int Height = InChunk.GetHeight();
	for ( int Y = 0; Y < Height; ++Y )
	{
		for ( int X = 0; X < Width; ++X )
		{
			std::uint8_t* const Pixel = OutRgba + (static_cast<size_t>(Y) * static_cast<size_t>(Width) + static_cast<size_t>(X)) * 4;
			const float MapX = static_cast<float>(InChunk.FirstColumn) + (static_cast<float>(X - InChunk.PadPixels) + 0.5f) / CellPixels;
			const float MapY = static_cast<float>(InChunk.FirstRow) + (static_cast<float>(Y - InChunk.PadPixels) + 0.5f) / CellPixels;
			if constexpr ( std::is_same_v<TSampler, MergeFieldCache> )
			{
				if ( InField.IsClearlyEmpty(MapX, MapY, Settings) )
				{
					Pixel[0] = Pixel[1] = Pixel[2] = Pixel[3] = 0;
					continue;
				}
			}
			const float JitterX = (Noise(MapX * JitterFrequency, MapY * JitterFrequency) - 0.5f) * JitterRange;
			const float JitterY = (Noise(MapX * JitterFrequency + 17.0f, MapY * JitterFrequency + 5.0f) - 0.5f) * JitterRange;
			const MergeSample Sample = InField.Sample(MapX, MapY, JitterX, JitterY, Settings);
			if ( !Sample.bInside )
			{
				Pixel[0] = Pixel[1] = Pixel[2] = Pixel[3] = 0;
				continue;
			}
			const float Depth = Sample.Coverage - Threshold;
			const float Edge = Clamp01(1.0f - Depth / EdgeBand);
			const float TileX = MapX / static_cast<float>(Subdivision);
			const float TileY = MapY / static_cast<float>(Subdivision);
			Rgb Color = Fill(Sample.Material, TileX, TileY, Edge);
			if ( !IsFloorMaterial(Sample.Material) )
			{
				const float Face = FaceShade(InField, MapX, MapY) * EdgeLightShade(InField, MapX, MapY);
				Color = { Color.R * Face, Color.G * Face, Color.B * Face };
			}
			const float Rim = 1.0f - RimShade + RimShade * Clamp01(Depth / RimDepth);
			Color = { Color.R * Rim, Color.G * Rim, Color.B * Rim };
			AddOverlay(Color, TileX, TileY, FloorToInt(TileX * TextureUnits), FloorToInt(TileY * TextureUnits), Edge);
			Pixel[0] = static_cast<std::uint8_t>(std::clamp(Color.R, 0.0f, 255.0f));
			Pixel[1] = static_cast<std::uint8_t>(std::clamp(Color.G, 0.0f, 255.0f));
			Pixel[2] = static_cast<std::uint8_t>(std::clamp(Color.B, 0.0f, 255.0f));
			Pixel[3] = static_cast<std::uint8_t>(255.0f * Clamp01((Depth + SoftEdge) / SoftEdge));
		}
	}
}

template <typename TSampler>
float TileMergeBaker::EdgeLightShade(const TSampler& InField, const float InX, const float InY) const
{
	const float Light = Settings.GetEdgeLight();
	if ( Light <= 0.0f )
	{
		return 1.0f;
	}
	const float Threshold = CoverageThreshold;
	const float Round = Settings.GetRoundness();
	const float Step = EdgeLightPixels / TextureUnits * static_cast<float>(Subdivision);
	const bool bTopOpen = InField.GetCoverage(InX, InY - Step, Round) < Threshold;
	const bool bLeftOpen = InField.GetCoverage(InX - Step, InY, Round) < Threshold;
	return bTopOpen || bLeftOpen ? 1.0f + Light : 1.0f;
}

template <typename TSampler>
float TileMergeBaker::FaceShade(const TSampler& InField, const float InX, const float InY) const
{
	if ( !Settings.IsSideView() )
	{
		return 1.0f;
	}
	const float Threshold = CoverageThreshold;
	const float Round = Settings.GetRoundness();
	const float Face = Settings.GetSideFacePx() / TextureUnits * static_cast<float>(Subdivision);
	if ( InField.GetCoverage(InX, InY + Face, Round) >= Threshold )
	{
		return 1.0f;
	}
	float Near = 0.0f;
	float Far = Face;
	for ( int Step = 0; Step < FaceSearchSteps; ++Step )
	{
		const float Middle = (Near + Far) * 0.5f;
		if ( InField.GetCoverage(InX, InY + Middle, Round) >= Threshold )
		{
			Near = Middle;
		}
		else
		{
			Far = Middle;
		}
	}
	const float Position = Clamp01(1.0f - (Near + Far) * 0.5f / Face);
	if ( Position < LightLineEnd )
	{
		return LightLineShade;
	}
	if ( Position < CreaseEnd )
	{
		return CreaseShade;
	}
	if ( Position < FaceMidEnd )
	{
		return FaceTopShade + (FaceMidShade - FaceTopShade) * (Position - CreaseEnd) / (FaceMidEnd - CreaseEnd);
	}
	return FaceFootTopShade + (FaceFootBottomShade - FaceFootTopShade) * (Position - FaceMidEnd) / (1.0f - FaceMidEnd);
}

template <typename TSampler>
void TileMergeBaker::BakeShadowWith(const TSampler& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	const float Threshold = CoverageThreshold;
	const float Soft = Settings.GetShadowSoft();
	const float Round = Settings.GetRoundness();
	const float CellPixels = static_cast<float>(InChunk.CellPixels);
	const float ToField = static_cast<float>(Subdivision) / TextureUnits;
	const float ReachX = Settings.GetShadowOffsetPx() * ToField;
	const float ReachY = Settings.GetShadowLengthPx() * ToField;
	const float Peak = 255.0f * Settings.GetShadowOpacity();
	const int Width = InChunk.GetWidth();
	const int Height = InChunk.GetHeight();
	for ( int Y = 0; Y < Height; ++Y )
	{
		for ( int X = 0; X < Width; ++X )
		{
			std::uint8_t* const Pixel = OutRgba + (static_cast<size_t>(Y) * static_cast<size_t>(Width) + static_cast<size_t>(X)) * 4;
			const float MapX = static_cast<float>(InChunk.FirstColumn) + (static_cast<float>(X - InChunk.PadPixels) + 0.5f) / CellPixels;
			const float MapY = static_cast<float>(InChunk.FirstRow) + (static_cast<float>(Y - InChunk.PadPixels) + 0.5f) / CellPixels;
			const float Footprint = Clamp01((InField.GetCoverage(MapX, MapY, Round) - (Threshold - Soft)) / (2.0f * Soft));
			float Strongest = 0.0f;
			for ( int Tap = 0; Tap <= ShadowTaps && Strongest < 1.0f; ++Tap )
			{
				const float Along = static_cast<float>(Tap) / static_cast<float>(ShadowTaps);
				const float Coverage = InField.GetCoverage(MapX - ReachX * Along, MapY - ReachY * Along, Round);
				Strongest = std::max(Strongest, Clamp01((Coverage - (Threshold - Soft)) / (2.0f * Soft)));
			}
			Pixel[0] = Pixel[1] = Pixel[2] = 0;
			Pixel[3] = static_cast<std::uint8_t>(Peak * Strongest * (1.0f - Footprint));
		}
	}
}

void TileMergeBaker::BakeCached(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	const MergeFieldCache Cache(InField, InChunk.FirstColumn, InChunk.FirstRow, InChunk.FirstColumn + InChunk.Columns, InChunk.FirstRow + InChunk.Rows, ComputeReachCells());
	BakeWith(Cache, InChunk, OutRgba);
}

void TileMergeBaker::BakeShadowCached(const TileMergeField& InField, const MergeChunk& InChunk, std::uint8_t* const OutRgba) const
{
	const MergeFieldCache Cache(InField, InChunk.FirstColumn, InChunk.FirstRow, InChunk.FirstColumn + InChunk.Columns, InChunk.FirstRow + InChunk.Rows, ComputeReachCells());
	BakeShadowWith(Cache, InChunk, OutRgba);
}
} // namespace game::model

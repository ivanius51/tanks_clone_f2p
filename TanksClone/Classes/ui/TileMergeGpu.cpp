#include "ui/TileMergeGpu.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "core/GameConstants.h"
#include "ui/TileMergeShaders.h"

namespace game::view::cocos
{
namespace
{
constexpr unsigned SeedPeriod = 1009;
constexpr float SeedScale = 0.37f;
constexpr float SeedSecondScale = 0.61f;

cocos2d::GLProgramState* MakeState(const std::string& InFragment)
{
	cocos2d::GLProgram* const Program = cocos2d::GLProgram::createWithByteArrays(cocos2d::ccPositionTextureColor_noMVP_vert, InFragment.c_str());
	if ( !Program )
	{
		return nullptr;
	}
	GLint Linked = GL_FALSE;
	glGetProgramiv(Program->getProgram(), GL_LINK_STATUS, &Linked);
	if ( Linked != GL_TRUE )
	{
		cocos2d::log("GPUMERGE shader failed: %s | %s", Program->getFragmentShaderLog().c_str(), Program->getProgramLog().c_str());
		return nullptr;
	}
	cocos2d::GLProgramState* const State = cocos2d::GLProgramState::create(Program);
	State->retain();
	return State;
}

cocos2d::Texture2D* MakeTexture(const std::vector<std::uint8_t>& InPixels, const int InWidth, const int InHeight)
{
	cocos2d::Texture2D* const Texture = new cocos2d::Texture2D();
	Texture->initWithData(InPixels.data(), static_cast<ssize_t>(InPixels.size()), cocos2d::Texture2D::PixelFormat::RGBA8888, InWidth, InHeight, cocos2d::Size(static_cast<float>(InWidth), static_cast<float>(InHeight)));
	const cocos2d::Texture2D::TexParams Params{ GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE };
	Texture->setTexParameters(Params);
	return Texture;
}
}

TileMergeGpu* TileMergeGpu::Create(const model::TileMergeSettings& InSettings, const unsigned InSeed, const int InSubdivision, const int InFieldColumns, const int InFieldRows, cocos2d::Node* const InColorParent)
{
	TileMergeGpu* const Gpu = new TileMergeGpu(InSeed, InSubdivision, InFieldColumns, InFieldRows);
	if ( !Gpu->Build(InSettings, InColorParent) )
	{
		delete Gpu;
		return nullptr;
	}
	return Gpu;
}

TileMergeGpu::TileMergeGpu(const unsigned InSeed, const int InSubdivision, const int InFieldColumns, const int InFieldRows)
	: Seed(InSeed)
	, Subdivision(InSubdivision)
	, Columns(InFieldColumns)
	, Rows(InFieldRows)
{
}

TileMergeGpu::~TileMergeGpu()
{
	if ( ColorQuad )
	{
		ColorQuad->removeFromParent();
	}
	if ( ShadowQuad )
	{
		ShadowQuad->removeFromParent();
	}
	for ( cocos2d::GLProgramState* const State : { ColorState, ShadowState } )
	{
		if ( State )
		{
			State->release();
		}
	}
	for ( cocos2d::Texture2D* const Texture : { FieldA, FieldB, White } )
	{
		if ( Texture )
		{
			Texture->release();
		}
	}
}

bool TileMergeGpu::Build(const model::TileMergeSettings& InSettings, cocos2d::Node* const InColorParent)
{
	Settings = InSettings;
	ColorState = MakeState(std::string(TileMergeGlslHeader) + TileMergeGlslMaterials + TileMergeGlslColorMain);
	if ( !ColorState )
	{
		return false;
	}
	const std::vector<std::uint8_t> Zero(static_cast<size_t>(Columns) * static_cast<size_t>(Rows) * 4, 0);
	FieldA = MakeTexture(Zero, Columns, Rows);
	FieldB = MakeTexture(Zero, Columns, Rows);
	White = MakeTexture(std::vector<std::uint8_t>(16, 255), 2, 2);
	ColorState->setUniformTexture("u_field0", FieldA);
	ColorState->setUniformTexture("u_field1", FieldB);
	SetCommonUniforms(ColorState, Settings);
	ColorQuad = MakeQuad(InColorParent, ColorState);
	return true;
}

cocos2d::Sprite* TileMergeGpu::MakeQuad(cocos2d::Node* const InParent, cocos2d::GLProgramState* const InState) const
{
	cocos2d::Sprite* const Quad = cocos2d::Sprite::createWithTexture(White);
	Quad->setAnchorPoint(cocos2d::Vec2::ZERO);
	const float FieldCellSize = TileSize / static_cast<float>(Subdivision);
	const cocos2d::Size Points = Quad->getContentSize();
	Quad->setScale(static_cast<float>(Columns) * FieldCellSize / Points.width, static_cast<float>(Rows) * FieldCellSize / Points.height);
	Quad->setPosition(0.0f, 0.0f);
	Quad->setBlendFunc(cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED);
	Quad->setGLProgramState(InState);
	InParent->addChild(Quad);
	return Quad;
}

void TileMergeGpu::SetCommonUniforms(cocos2d::GLProgramState* const InState, const model::TileMergeSettings& InSettings) const
{
	const float SeedPart = static_cast<float>(Seed % SeedPeriod) * SeedScale;
	InState->setUniformVec2("u_cells", cocos2d::Vec2(static_cast<float>(Columns), static_cast<float>(Rows)));
	InState->setUniformVec2("u_seedOffset", cocos2d::Vec2(SeedPart, SeedPart * SeedSecondScale));
	InState->setUniformVec4("u_params0", cocos2d::Vec4(0.0f, static_cast<float>(Subdivision), InSettings.ResolveThreshold(Seed), InSettings.GetThreshold()));
	InState->setUniformVec4("u_params1", cocos2d::Vec4(InSettings.GetRoundness(), InSettings.GetFloorHardBorder(), InSettings.GetHardBorder(), InSettings.GetEdgeLight()));
	InState->setUniformVec4("u_params2", cocos2d::Vec4(InSettings.GetSideFacePx(), InSettings.GetPlaster(), InSettings.GetCracks(), InSettings.GetOverlayAmount()));
	const std::array<std::uint8_t, 3>& Overlay = InSettings.GetOverlayColor();
	InState->setUniformVec3("u_overlayColor", cocos2d::Vec3(static_cast<float>(Overlay[0]) / 255.0f, static_cast<float>(Overlay[1]) / 255.0f, static_cast<float>(Overlay[2]) / 255.0f));
	std::array<float, 8> Hard{};
	std::array<float, 8> Sharp{};
	for ( int Index = 0; Index < model::MergeMaterialCount; ++Index )
	{
		const model::MergeMaterial Kind = static_cast<model::MergeMaterial>(Index);
		Hard[static_cast<size_t>(Index)] = model::IsFloorMaterial(Kind) ? InSettings.GetFloorHardBorder() : (InSettings.GetMaterial(Kind).bHardBorder ? InSettings.GetHardBorder() : 0.0f);
		Sharp[static_cast<size_t>(Index)] = InSettings.GetMaterial(Kind).CornerSharpness;
	}
	InState->setUniformVec4("u_hardA", cocos2d::Vec4(Hard[0], Hard[1], Hard[2], Hard[3]));
	InState->setUniformVec4("u_hardB", cocos2d::Vec4(Hard[4], Hard[5], Hard[6], 0.0f));
	InState->setUniformVec4("u_sharpA", cocos2d::Vec4(Sharp[0], Sharp[1], Sharp[2], Sharp[3]));
	InState->setUniformVec4("u_sharpB", cocos2d::Vec4(Sharp[4], Sharp[5], Sharp[6], 0.0f));
}

void TileMergeGpu::SetShadowParent(cocos2d::Node* const InParent)
{
	if ( ShadowQuad )
	{
		ShadowQuad->removeFromParent();
		ShadowQuad = nullptr;
	}
	if ( !InParent || !Settings.IsShadowEnabled() )
	{
		return;
	}
	if ( !ShadowState )
	{
		ShadowState = MakeState(std::string(TileMergeGlslHeader) + TileMergeGlslShadowMain);
	}
	if ( !ShadowState )
	{
		return;
	}
	ShadowState->setUniformTexture("u_field0", FieldA);
	ShadowState->setUniformTexture("u_field1", FieldB);
	SetCommonUniforms(ShadowState, Settings);
	ApplyShadowSettings(Settings);
	ShadowQuad = MakeQuad(InParent, ShadowState);
}

void TileMergeGpu::ApplyShadowSettings(const model::TileMergeSettings& InSettings)
{
	if ( !ShadowState )
	{
		return;
	}
	const float ToField = static_cast<float>(Subdivision) / 64.0f;
	ShadowState->setUniformVec4("u_shadow", cocos2d::Vec4(InSettings.GetShadowOffsetPx() * ToField, InSettings.GetShadowLengthPx() * ToField, InSettings.GetShadowSoft(), InSettings.GetShadowOpacity()));
}

void TileMergeGpu::ApplySettings(const model::TileMergeSettings& InSettings)
{
	Settings = InSettings;
	SetCommonUniforms(ColorState, Settings);
	if ( ShadowState )
	{
		SetCommonUniforms(ShadowState, Settings);
		ApplyShadowSettings(Settings);
	}
}

void TileMergeGpu::Upload(const model::TileMergeField& InField)
{
	const size_t Count = static_cast<size_t>(Columns) * static_cast<size_t>(Rows);
	std::vector<std::uint8_t> First(Count * 4);
	std::vector<std::uint8_t> Second(Count * 4);
	const auto Pack = [](const float InValue) { return static_cast<std::uint8_t>(std::clamp(InValue, 0.0f, 1.0f) * 255.0f + 0.5f); };
	for ( int Row = 0; Row < Rows; ++Row )
	{
		for ( int Column = 0; Column < Columns; ++Column )
		{
			const size_t Texel = (static_cast<size_t>(Row) * static_cast<size_t>(Columns) + static_cast<size_t>(Column)) * 4;
			for ( int Material = 0; Material < model::MergeMaterialCount; ++Material )
			{
				const std::uint8_t Value = Pack(InField.GetValue(static_cast<model::MergeMaterial>(Material), Column, Row));
				std::vector<std::uint8_t>& Target = Material < 4 ? First : Second;
				Target[Texel + static_cast<size_t>(Material % 4)] = Value;
			}
		}
	}
	FieldA->updateWithData(First.data(), 0, 0, Columns, Rows);
	FieldB->updateWithData(Second.data(), 0, 0, Columns, Rows);
}
} // namespace game::view::cocos

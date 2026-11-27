#pragma once

#include "cocos2d.h"

#include "model/TileMergeField.h"

namespace game::view::cocos
{
/**
 * The GPU path of the wall merge. The merged shape is not baked into textures: a quad over the whole map is drawn with
 * a fragment shader (ui/TileMergeShaders.h) that reads the field from two small textures, one texel per field cell, so
 * destroying a wall only re-uploads those textures (tens of kilobytes) and costs the CPU nothing, whatever the pixel
 * density. The shader needs highp floats; Create() returns nullptr where it does not compile, and the view then uses a
 * CPU baker.
 */
class TileMergeGpu
{
public:
	static TileMergeGpu* Create(const model::TileMergeSettings& InSettings, unsigned InSeed, int InSubdivision, int InFieldColumns, int InFieldRows, cocos2d::Node* InColorParent);
	~TileMergeGpu();

	/** Milliseconds to draw one full-map frame of the shader into an offscreen target; -1 where the shader does not build. Needs a GL context. */
	static float MeasureFrameMs(const model::TileMergeSettings& InSettings);

	void SetShadowParent(cocos2d::Node* InParent);
	void ApplySettings(const model::TileMergeSettings& InSettings);
	/** Copies the field's current values into the cell textures. */
	void Upload(const model::TileMergeField& InField);

private:
	TileMergeGpu(unsigned InSeed, int InSubdivision, int InFieldColumns, int InFieldRows);

	bool Build(const model::TileMergeSettings& InSettings, cocos2d::Node* InColorParent);
	cocos2d::Sprite* MakeQuad(cocos2d::Node* InParent, cocos2d::GLProgramState* InState) const;
	void SetCommonUniforms(cocos2d::GLProgramState* InState, const model::TileMergeSettings& InSettings) const;
	void ApplyShadowSettings(const model::TileMergeSettings& InSettings);

	unsigned Seed;
	int Subdivision;
	int Columns;
	int Rows;
	cocos2d::Texture2D* FieldA = nullptr;
	cocos2d::Texture2D* FieldB = nullptr;
	cocos2d::Texture2D* White = nullptr;
	cocos2d::Sprite* ColorQuad = nullptr;
	cocos2d::Sprite* ShadowQuad = nullptr;
	cocos2d::GLProgramState* ColorState = nullptr;
	cocos2d::GLProgramState* ShadowState = nullptr;
	model::TileMergeSettings Settings;
};
} // namespace game::view::cocos

#pragma once

#include "cocos2d.h"

#include "model/TileMergeBaker.h"
#include "model/TileMergeField.h"
#include "templates/TContainers.h"
#include "templates/TUniquePtr.h"

namespace game::view::cocos
{
/**
 * Draws a TileMergeField as chunk sprites: the merged shape of the map's walls or floor modifiers, baked into RGBA
 * textures a few tiles wide and re-baked only where a cell changes, a few chunks per frame. This is the Cocos side of the
 * merge; the field and the baker know no engine. Cell rows count from the top of the map, like the level rows.
 */
class TileMergeView : public cocos2d::Node
{
public:
	static TileMergeView* Create(const model::TileMergeSettings& InSettings, int InColumns, int InRows, unsigned InSeed, int InSubdivision);

	int GetSubdivision() const { return Subdivision; }
	// Field cells are a tile divided into Subdivision x Subdivision; rows count from the top.
	void SetCell(int InColumn, int InRow, model::MergeMaterial InMaterial, float InAmount);
	bool ToCell(const cocos2d::Vec2& InPosition, int& OutColumn, int& OutRow) const;
	void BakeAll();
	void ApplySettings(const model::TileMergeSettings& InSettings);
	void SetShadowParent(cocos2d::Node* InParent);

	void update(float InDelta) override;

private:
	struct Chunk
	{
		model::MergeChunk Area;
		cocos2d::Texture2D* Texture = nullptr;
		cocos2d::Sprite* Image = nullptr;
		cocos2d::Texture2D* ShadowTexture = nullptr;
		cocos2d::Sprite* ShadowImage = nullptr;
		bool bDirty = false;
	};

	TileMergeView(const model::TileMergeSettings& InSettings, int InColumns, int InRows, unsigned InSeed, int InSubdivision);

	void ApplyLift();
	void MarkDirty(int InColumn, int InRow);
	void BakeChunk(Chunk& InChunk);
	void UploadLayer(const Chunk& InChunk, cocos2d::Node* InParent, cocos2d::Texture2D*& InOutTexture, cocos2d::Sprite*& InOutSprite);
	Chunk* FindChunk(int InColumn, int InRow);

	cocos2d::Node* ShadowParent = nullptr;
	model::TileMergeSettings Settings;
	model::TileMergeField Field;
	TUniquePtr<model::TileMergeBaker> Baker;
	TArray<Chunk> Chunks;
	TArray<std::uint8_t> Scratch;
	int Subdivision = 1;
	int ChunkColumns = 0;
	int ChunkRows = 0;
};
} // namespace game::view::cocos

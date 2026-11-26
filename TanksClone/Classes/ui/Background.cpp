#include "ui/Background.h"

#include <algorithm>
#include <numeric>
#include <random>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/ConfigNode.h"
#include "services/ConfigRefs.h"
#include "game/ResourceManifest.h"
#include "ui/Widgets.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{

namespace
{
	const cocos2d::Color4F FloorA(0.13f, 0.15f, 0.12f, 1.0f);
	const cocos2d::Color4F FloorB(0.15f, 0.17f, 0.14f, 1.0f);
	const cocos2d::Color4F FrameColor(0.4f, 0.45f, 0.52f, 1.0f);

	FPersistentString BackgroundConfig()
	{
		static const FPersistentString Path = config_refs::ResolveId(ids::configs::Background);
		return Path;
	}

	const TArray<int>& GroundVariantWeights()
	{
		static const TArray<int> Weights = []()
		{
			TArray<int> Read;
			const model::IConfigNode& Config = GameContext::Get<ConfigReader>().GetConfig(BackgroundConfig());
			Config.TryRead("ground_variant_weights", Read);
			if ( Read.IsEmpty() )
			{
				CCLOGERROR("Background: \"ground_variant_weights\" is missing in %s", std::string(BackgroundConfig().View()).c_str());
				Read.PushBack(1);
			}
			return Read;
		}();
		return Weights;
	}

	int PickGroundVariant(std::mt19937& InRandom)
	{
		const TArray<int>& Weights = GroundVariantWeights();
		int Roll = static_cast<int>(InRandom() % static_cast<unsigned>(std::max(1, std::accumulate(Weights.begin(), Weights.end(), 0))));
		int Variant = 1;
		for ( int Weight : Weights )
		{
			if ( Roll < Weight )
			{
				return Variant;
			}
			Roll -= Weight;
			++Variant;
		}
		return 1;
	}

	cocos2d::Node* CreateGroundTile(const std::string& InTileset, const int InRow, const int InColumn, std::mt19937& InRandom)
	{
		const std::string CurrentFrame = "ground_" + std::to_string(PickGroundVariant(InRandom));
		auto CurrentSprite = widgets::CreateFrameSprite(ResourceManifest::GetTilesetFrame(InTileset, CurrentFrame));
		if ( !CurrentSprite )
		{
			auto Plain = cocos2d::DrawNode::create();
			Plain->drawSolidRect(cocos2d::Vec2::ZERO, cocos2d::Vec2(TileSize, TileSize), (InRow + InColumn) % 2 ? FloorA : FloorB);
			Plain->setPosition(InColumn * TileSize, InRow * TileSize);
			return Plain;
		}
		CurrentSprite->setScale(TileSize / CurrentSprite->getContentSize().width);
		CurrentSprite->setPosition((InColumn + 0.5f) * TileSize, (InRow + 0.5f) * TileSize);
		return CurrentSprite;
	}
}

cocos2d::Node* CreateBackground(const unsigned InSeed, const std::string& InTileset, const int InColumns, const int InRows)
{
	auto CurrentRoot = cocos2d::Node::create();
	std::mt19937 Random(InSeed);

	for ( int Row = 0; Row < InRows; ++Row )
	{
		for ( int Column = 0; Column < InColumns; ++Column )
		{
			CurrentRoot->addChild(CreateGroundTile(InTileset, Row, Column, Random));
		}
	}

	auto CurrentFrame = cocos2d::DrawNode::create();
	CurrentFrame->drawRect(cocos2d::Vec2(1, 1), cocos2d::Vec2(InColumns * TileSize - 1, InRows * TileSize - 1), FrameColor);
	CurrentRoot->addChild(CurrentFrame, 1);
	return CurrentRoot;
}

} // namespace game::view::cocos

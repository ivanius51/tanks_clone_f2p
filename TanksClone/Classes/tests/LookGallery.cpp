#include "tests/LookGallery.h"

#include <cmath>
#include <string>
#include <vector>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/EnemyCatalog.h"
#include "game/LevelLoader.h"
#include "game/ObjectsFactory.h"
#include "game/ai/TankAi.h"
#include "game/objects/Tank.h"
#include "model/AiTactic.h"
#include "scenes/GameScene.h"

namespace tanks
{
namespace tests
{

namespace
{
	constexpr const char* GalleryConfig = "configs/test/look_gallery.json";
	constexpr float SetUpDelay = 1.0f;
	constexpr float LabelFontSize = 11.0f;
	constexpr int HitDamage = 1;

	struct GalleryState
	{
		std::string Label;
		float HealthShare = 1.0f;
		std::vector<cocos2d::Vec2> Hits;
		std::string Buff;
		std::string Tactic;
	};

	struct Gallery
	{
		std::string Level;
		std::string Enemy;
		int FirstColumn = 1;
		int FirstRow = 1;
		int Columns = 7;
		int StepTiles = 2;
		std::vector<GalleryState> States;
	};

	cocos2d::Vec2 ReadVec(const model::IConfigNode& InNode)
	{
		return cocos2d::Vec2(static_cast<float>(InNode.GetItem(0)->ReadNumber().value_or(0.0)), static_cast<float>(InNode.GetItem(1)->ReadNumber().value_or(0.0)));
	}

	Gallery LoadGallery()
	{
		Gallery Result;
		const model::IConfigNode& Root = GameContext::Get().GetManager<ConfigReader>().GetConfig(GalleryConfig);
		Result.Level = Root.GetValueOr("level", std::string());
		Result.Enemy = Root.GetValueOr("enemy", std::string("Basic"));
		Result.FirstColumn = static_cast<int>(Root.GetValueOr("first_column", static_cast<float>(Result.FirstColumn)));
		Result.FirstRow = static_cast<int>(Root.GetValueOr("first_row", static_cast<float>(Result.FirstRow)));
		Result.Columns = static_cast<int>(Root.GetValueOr("columns", static_cast<float>(Result.Columns)));
		Result.StepTiles = static_cast<int>(Root.GetValueOr("step_tiles", static_cast<float>(Result.StepTiles)));
		if ( const model::IConfigNode* const States = Root.FindChild("states") )
		{
			for ( size_t Index = 0; Index < States->GetItemCount(); ++Index )
			{
				const model::IConfigNode& Node = *States->GetItem(Index);
				GalleryState State;
				State.Label = Node.GetValueOr("label", std::string());
				State.HealthShare = Node.GetValueOr("health", State.HealthShare);
				State.Buff = Node.GetValueOr("buff", std::string());
				State.Tactic = Node.GetValueOr("tactic", std::string());
				if ( const model::IConfigNode* const Hits = Node.FindChild("hits") )
				{
					for ( size_t HitIndex = 0; HitIndex < Hits->GetItemCount(); ++HitIndex )
					{
						State.Hits.push_back(ReadVec(*Hits->GetItem(HitIndex)));
					}
				}
				Result.States.push_back(State);
			}
		}
		return Result;
	}

	model::AiTactic FindTactic(const std::string& InName)
	{
		for ( size_t Index = 0; Index < static_cast<size_t>(model::AiTactic::Count); ++Index )
		{
			if ( model::GetTacticName(static_cast<model::AiTactic>(Index)) == InName )
			{
				return static_cast<model::AiTactic>(Index);
			}
		}
		return model::AiTactic::Wander;
	}

	cocos2d::Vec2 TileCenter(const int InColumn, const int InRow)
	{
		return cocos2d::Vec2((InColumn + 0.5f) * TileSize, (GridRows - 1 - InRow + 0.5f) * TileSize);
	}

	cocos2d::Vec2 FindPlayerTile(const LevelData& InLevel)
	{
		for ( size_t Row = 0; Row < InLevel.Rows.size(); ++Row )
		{
			const size_t Column = InLevel.Rows[Row].find(tiles::PlayerSpawn);
			if ( Column != std::string::npos )
			{
				return TileCenter(static_cast<int>(Column), static_cast<int>(Row));
			}
		}
		return cocos2d::Vec2::ZERO;
	}

	void ShowState(Tank& InTank, const GalleryState& InState)
	{
		InTank.SetAiSuspended(true);
		for ( const cocos2d::Vec2& Direction : InState.Hits )
		{
			InTank.ApplyDamageFrom(HitDamage, Team::Player, Direction);
		}
		InTank.SetHealth(std::max(1, static_cast<int>(std::round(InTank.GetMaxHealth() * InState.HealthShare))));
		if ( !InState.Buff.empty() )
		{
			if ( const BuffDef* const Buff = GameContext::Get().GetManager<BuffCatalog>().Find(InState.Buff) )
			{
				BuffDef Permanent = *Buff;
				Permanent.Duration = -1.0f;
				InTank.GetBuffs().Add(Permanent);
			}
		}
		if ( !InState.Tactic.empty() && InTank.GetAi() )
		{
			InTank.GetAi()->ForceTactic(FindTactic(InState.Tactic));
		}
	}

	void SetUpGallery(const Gallery& InGallery, const LevelData& InLevel)
	{
		Tank* Player = nullptr;
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&Player](Tank& InTank)
		{
			if ( InTank.GetTeam() == Team::Player )
			{
				Player = &InTank;
			}
		});
		const EnemyDef* const Enemy = GameContext::Get().GetManager<EnemyCatalog>().Find(InGallery.Enemy);
		if ( !Player || !Player->GetVisualNode() || !Enemy )
		{
			return;
		}
		cocos2d::Node* const World = Player->GetVisualNode()->getParent();
		const cocos2d::Vec2 Offset = Player->GetVisualNode()->getPosition() - FindPlayerTile(InLevel);
		for ( size_t Index = 0; Index < InGallery.States.size(); ++Index )
		{
			const int Column = InGallery.FirstColumn + static_cast<int>(Index) % InGallery.Columns * InGallery.StepTiles;
			const int Row = InGallery.FirstRow + static_cast<int>(Index) / InGallery.Columns * InGallery.StepTiles;
			const cocos2d::Vec2 Position = TileCenter(Column, Row) + Offset;
			cocos2d::ValueMap Overrides;
			Overrides["position"] = cocos2d::ValueVector{ cocos2d::Value(Position.x), cocos2d::Value(Position.y) };
			Tank* const CurrentTank = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(Enemy->ConfigId, World, Overrides);
			if ( !CurrentTank )
			{
				continue;
			}
			ShowState(*CurrentTank, InGallery.States[Index]);
			cocos2d::Label* const Caption = cocos2d::Label::createWithSystemFont(InGallery.States[Index].Label, "Arial", LabelFontSize);
			Caption->setPosition(Position - cocos2d::Vec2(0.0f, TileSize * 0.7f));
			World->addChild(Caption, 100);
		}
	}
}

void PlayLookGallery()
{
	const Gallery CurrentGallery = LoadGallery();
	LevelData CurrentLevel;
	LevelLoader::Load(CurrentGallery.Level, CurrentLevel);
	cocos2d::Director::getInstance()->replaceScene(view::cocos::GameScene::CreateForTest(CurrentLevel, false));
	cocos2d::Director::getInstance()->getScheduler()->schedule([CurrentGallery, CurrentLevel](float)
	{
		SetUpGallery(CurrentGallery, CurrentLevel);
	}, cocos2d::Director::getInstance(), 0.0f, 0, SetUpDelay, false, "look_gallery_set_up");
}

} // namespace tests
} // namespace tanks

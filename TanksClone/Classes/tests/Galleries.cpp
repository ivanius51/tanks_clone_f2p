#include "tests/Galleries.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/Effects.h"
#include "game/EnemyCatalog.h"
#include "game/LevelLoader.h"
#include "game/ObjectsFactory.h"
#include "game/ai/TankAi.h"
#include "game/components/Buff.h"
#include "game/objects/Tank.h"
#include "model/AiTactic.h"
#include "scenes/GameScene.h"
#include "services/ConfigRefs.h"
#include "services/StringsManager.h"
#include "ui/Widgets.h"

namespace tanks
{
namespace tests
{

namespace
{
	constexpr float SetUpDelay = 1.0f;
	constexpr float DefaultEffectInterval = 0.7f;
	constexpr float LabelFontSize = 11.0f;
	constexpr float LabelDrop = 0.7f;   // tiles under the tank or effect
	constexpr float FaceDistance = 0.5f; // tiles from a tile centre to its face
	constexpr int LabelZ = 100;
	constexpr int HitDamage = 1;
	const char* const SetUpKey = "gallery_set_up";
	const char* const EffectsKey = "gallery_effects";
	const char* const WrecksKey = "gallery_wrecks";
	constexpr int PickerZ = 1000;

	struct GalleryState
	{
		std::string Label;
		float HealthShare = 1.0f;
		std::vector<cocos2d::Vec2> Hits;
		std::string Buff;
		std::string Tactic;
	};

	struct GalleryEffect
	{
		std::string Label;
		std::string Effect;
		int Column = 0;
		int Row = 0;
		std::string Enemy;                  // a tank to stand on the tile, for effects that hit armor
		std::vector<cocos2d::Vec2> Shots;   // shot directions to replay in turn
	};

	struct Grid
	{
		int FirstColumn = 1;
		int FirstRow = 1;
		int Columns = 7;
		int StepTiles = 2;

		void Cell(const size_t InIndex, int& OutColumn, int& OutRow) const
		{
			OutColumn = FirstColumn + static_cast<int>(InIndex) % Columns * StepTiles;
			OutRow = FirstRow + static_cast<int>(InIndex) / Columns * StepTiles;
		}
	};

	struct Gallery
	{
		std::string Level;
		std::string Enemy;
		Grid Layout;
		std::vector<GalleryState> States;
		bool bAllEnemies = false;
		bool bWrecks = false;
		float WreckInterval = 6.0f;
		float WreckDelay = 1.0f;
		std::vector<GalleryEffect> Effects;
		float EffectInterval = DefaultEffectInterval;
	};

	cocos2d::Vec2 ReadVec(const model::IConfigNode& InNode)
	{
		return cocos2d::Vec2(static_cast<float>(InNode.GetItem(0)->ReadNumber().value_or(0.0)), static_cast<float>(InNode.GetItem(1)->ReadNumber().value_or(0.0)));
	}

	std::vector<cocos2d::Vec2> ReadVecList(const model::IConfigNode* const InList)
	{
		std::vector<cocos2d::Vec2> Result;
		for ( size_t Index = 0; InList && Index < InList->GetItemCount(); ++Index )
		{
			Result.push_back(ReadVec(*InList->GetItem(Index)));
		}
		return Result;
	}

	int ReadInt(const model::IConfigNode& InNode, std::string_view InKey, const int InDefault)
	{
		return static_cast<int>(InNode.GetValueOr(InKey, static_cast<float>(InDefault)));
	}

	Gallery LoadGallery(const std::string& InPath)
	{
		Gallery Result;
		const model::IConfigNode& Root = GameContext::Get().GetManager<ConfigReader>().GetConfig(InPath);
		Result.Level = config_refs::ResolveName(Root.GetValueOr("level", std::string()));
		Result.Enemy = Root.GetValueOr("enemy", std::string("Basic"));
		Result.Layout.FirstColumn = ReadInt(Root, "first_column", Result.Layout.FirstColumn);
		Result.Layout.FirstRow = ReadInt(Root, "first_row", Result.Layout.FirstRow);
		Result.Layout.Columns = std::max(1, ReadInt(Root, "columns", Result.Layout.Columns));
		Result.Layout.StepTiles = ReadInt(Root, "step_tiles", Result.Layout.StepTiles);
		Result.bAllEnemies = Root.GetValueOr("all_enemies", false);
		Result.bWrecks = Root.GetValueOr("wrecks", false);
		Result.WreckInterval = Root.GetValueOr("wreck_interval", Result.WreckInterval);
		Result.WreckDelay = Root.GetValueOr("wreck_delay", Result.WreckDelay);
		Result.EffectInterval = Root.GetValueOr("effect_interval", Result.EffectInterval);
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
				State.Hits = ReadVecList(Node.FindChild("hits"));
				Result.States.push_back(State);
			}
		}
		if ( const model::IConfigNode* const Effects = Root.FindChild("effects") )
		{
			for ( size_t Index = 0; Index < Effects->GetItemCount(); ++Index )
			{
				const model::IConfigNode& Node = *Effects->GetItem(Index);
				GalleryEffect Effect;
				Effect.Label = Node.GetValueOr("label", std::string());
				Effect.Effect = Node.GetValueOr("effect", std::string());
				Effect.Enemy = Node.GetValueOr("enemy", std::string());
				if ( const model::IConfigNode* const Tile = Node.FindChild("tile") )
				{
					const cocos2d::Vec2 TileVec = ReadVec(*Tile);
					Effect.Column = static_cast<int>(TileVec.x);
					Effect.Row = static_cast<int>(TileVec.y);
				}
				Effect.Shots = ReadVecList(Node.FindChild("shots"));
				if ( Effect.Shots.empty() )
				{
					Effect.Shots = { { 0.0f, -1.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f } };
				}
				Result.Effects.push_back(Effect);
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

	/** Places things on the gallery level: positions are level tiles, lined up with the map through the player tank. */
	class GalleryBuilder
	{
	public:
		GalleryBuilder(cocos2d::Node& InWorld, const cocos2d::Vec2& InOffset)
			: World(InWorld), Offset(InOffset)
		{
		}

		cocos2d::Vec2 At(const int InColumn, const int InRow) const
		{
			return TileCenter(InColumn, InRow) + Offset;
		}

		Tank* AddTank(const std::string& InEnemyId, const cocos2d::Vec2& InPosition) const
		{
			const EnemyDef* const Enemy = GameContext::Get().GetManager<EnemyCatalog>().Find(InEnemyId);
			if ( !Enemy )
			{
				return nullptr;
			}
			cocos2d::ValueMap Overrides;
			Overrides["position"] = cocos2d::ValueVector{ cocos2d::Value(InPosition.x), cocos2d::Value(InPosition.y) };
			Tank* const CurrentTank = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(Enemy->ConfigId, &World, Overrides);
			if ( CurrentTank )
			{
				CurrentTank->SetFrozen(true);
			}
			return CurrentTank;
		}

		void AddLabel(const std::string& InText, const cocos2d::Vec2& InPosition) const
		{
			cocos2d::Label* const Caption = cocos2d::Label::createWithSystemFont(InText, "Arial", LabelFontSize);
			Caption->setPosition(InPosition - cocos2d::Vec2(0.0f, TileSize * LabelDrop));
			World.addChild(Caption, LabelZ);
		}

		cocos2d::Node& GetWorld() const
		{
			return World;
		}

	private:
		cocos2d::Node& World;
		cocos2d::Vec2 Offset;
	};

	void AddStates(const Gallery& InGallery, const GalleryBuilder& InBuilder)
	{
		for ( size_t Index = 0; Index < InGallery.States.size(); ++Index )
		{
			int Column = 0;
			int Row = 0;
			InGallery.Layout.Cell(Index, Column, Row);
			const cocos2d::Vec2 Position = InBuilder.At(Column, Row);
			if ( Tank* const CurrentTank = InBuilder.AddTank(InGallery.Enemy, Position) )
			{
				ShowState(*CurrentTank, InGallery.States[Index]);
				InBuilder.AddLabel(InGallery.States[Index].Label, Position);
			}
		}
	}

	void AddAllEnemies(const Gallery& InGallery, const GalleryBuilder& InBuilder)
	{
		const auto& Enemies = GameContext::Get().GetManager<EnemyCatalog>().GetAll();
		for ( size_t Index = 0; Index < Enemies.size(); ++Index )
		{
			int Column = 0;
			int Row = 0;
			InGallery.Layout.Cell(Index, Column, Row);
			const cocos2d::Vec2 Position = InBuilder.At(Column, Row);
			if ( InBuilder.AddTank(Enemies[Index].Id, Position) )
			{
				InBuilder.AddLabel(Enemies[Index].Id, Position);
			}
		}
	}

	// Every enemy type, killed again and again: even rows by an exact kill, odd rows by a big overkill (a 1 HP wreck).
	void AddWrecks(const Gallery& InGallery, const GalleryBuilder& InBuilder)
	{
		const auto& Enemies = GameContext::Get().GetManager<EnemyCatalog>().GetAll();
		for ( size_t Index = 0; Index < Enemies.size(); ++Index )
		{
			int Column = 0;
			int Row = 0;
			InGallery.Layout.Cell(Index, Column, Row);
			InBuilder.AddLabel(Enemies[Index].Id, InBuilder.At(Column, Row));
		}
		auto Round = std::make_shared<int>(0);
		auto Spawn = [InGallery, InBuilder, Round](float)
		{
			const auto& All = GameContext::Get().GetManager<EnemyCatalog>().GetAll();
			for ( size_t Index = 0; Index < All.size(); ++Index )
			{
				int Column = 0;
				int Row = 0;
				InGallery.Layout.Cell(Index, Column, Row);
				Tank* const Victim = InBuilder.AddTank(All[Index].Id, InBuilder.At(Column, Row));
				if ( !Victim || !Victim->GetVisualNode() )
				{
					continue;
				}
				const bool bOverkill = (*Round + Row / InGallery.Layout.StepTiles) % 2 == 1;
				Victim->GetVisualNode()->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(InGallery.WreckDelay),
					cocos2d::CallFunc::create([Victim, bOverkill]()
					{
						Victim->SetHealth(1);
						Victim->ApplyDamage(1 + (bOverkill ? Victim->GetMaxHealth() : 0), Team::Player);
					}), nullptr));
			}
			++*Round;
		};
		Spawn(0.0f);
		InBuilder.GetWorld().schedule(Spawn, InGallery.WreckInterval, WrecksKey);
	}

	void AddEffects(const Gallery& InGallery, const GalleryBuilder& InBuilder)
	{
		if ( InGallery.Effects.empty() )
		{
			return;
		}
		for ( const GalleryEffect& Effect : InGallery.Effects )
		{
			const cocos2d::Vec2 Position = InBuilder.At(Effect.Column, Effect.Row);
			if ( !Effect.Enemy.empty() )
			{
				InBuilder.AddTank(Effect.Enemy, Position);
			}
			InBuilder.AddLabel(Effect.Label, Position);
		}
		auto Turn = std::make_shared<size_t>(0);
		cocos2d::Node* const World = &InBuilder.GetWorld();
		World->schedule([InGallery, InBuilder, Turn](float)
		{
			for ( const GalleryEffect& Effect : InGallery.Effects )
			{
				const cocos2d::Vec2 Shot = Effect.Shots[*Turn % Effect.Shots.size()];
				const cocos2d::Vec2 Face = InBuilder.At(Effect.Column, Effect.Row) - Shot * (TileSize * FaceDistance);
				effects::Play(Effect.Effect, &InBuilder.GetWorld(), Face, Shot);
			}
			++*Turn;
		}, InGallery.EffectInterval, EffectsKey);
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
		if ( !Player || !Player->GetVisualNode() || !Player->GetVisualNode()->getParent() )
		{
			return;
		}
		const GalleryBuilder Builder(*Player->GetVisualNode()->getParent(), Player->GetVisualNode()->getPosition() - FindPlayerTile(InLevel));
		AddStates(InGallery, Builder);
		if ( InGallery.bAllEnemies )
		{
			AddAllEnemies(InGallery, Builder);
		}
		if ( InGallery.bWrecks )
		{
			AddWrecks(InGallery, Builder);
		}
		AddEffects(InGallery, Builder);
	}
}

std::vector<GalleryEntry> Galleries::GetAll()
{
	std::vector<GalleryEntry> Result;
	const model::IConfigNode& Root = GameContext::Get().GetManager<ConfigReader>().GetConfig(config_refs::ResolveId(ids::configs::Galleries));
	if ( const model::IConfigNode* const List = Root.FindChild("galleries") )
	{
		for ( size_t Index = 0; Index < List->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Node = *List->GetItem(Index);
			Result.push_back({ Node.GetValueOr("id", std::string()), Node.GetValueOr("name", std::string()), config_refs::ResolveName(Node.GetValueOr("config", std::string())) });
		}
	}
	return Result;
}

void Galleries::Play(const std::string_view InId)
{
	const std::vector<GalleryEntry> All = GetAll();
	const auto Found = std::find_if(All.begin(), All.end(), [InId](const GalleryEntry& InEntry) { return InEntry.Id == InId; });
	if ( Found == All.end() )
	{
		CCLOGERROR("Galleries: no gallery %s", std::string(InId).c_str());
		return;
	}
	const Gallery CurrentGallery = LoadGallery(Found->Config);
	LevelData CurrentLevel;
	LevelLoader::Load(CurrentGallery.Level, CurrentLevel);
	cocos2d::Scene* const Scene = view::cocos::GameScene::CreateForTest(CurrentLevel, false);
	if ( auto* const Game = view::cocos::SceneCast<view::cocos::GameScene>(Scene) )
	{
		Game->AddPauseButton(GameContext::Get().GetManager<StringsManager>().GetString("DEBUG_GALLERIES"), [Game]()
		{
			ShowPicker(*Game, PickerZ);
		});
	}
	cocos2d::Director::getInstance()->replaceScene(Scene);
	cocos2d::Scheduler* const Scheduler = cocos2d::Director::getInstance()->getScheduler();
	Scheduler->unschedule(SetUpKey, cocos2d::Director::getInstance());
	Scheduler->schedule([CurrentGallery, CurrentLevel](float)
	{
		SetUpGallery(CurrentGallery, CurrentLevel);
	}, cocos2d::Director::getInstance(), 0.0f, 0, SetUpDelay, false, SetUpKey);
}

void Galleries::ShowPicker(cocos2d::Node& InScene, const int InZOrder)
{
	auto& Strings = GameContext::Get().GetManager<StringsManager>();
	std::vector<view::cocos::widgets::Overlay::ButtonSpec> Buttons;
	for ( const GalleryEntry& Entry : GetAll() )
	{
		const std::string Id = Entry.Id;
		Buttons.push_back({ Strings.GetString(Entry.NameKey), [Id]() { Play(Id); } });
	}
	auto Shown = std::make_shared<cocos2d::Node*>(nullptr);
	Buttons.push_back({ Strings.GetString("BACK_DEBUG"), [Shown]() { (*Shown)->removeFromParent(); } });
	*Shown = view::cocos::widgets::Overlay::create(Strings.GetString("GALLERIES_TITLE"), Strings.GetString("GALLERIES_TEXT"), Buttons);
	InScene.addChild(*Shown, InZOrder);
}

} // namespace tests
} // namespace tanks

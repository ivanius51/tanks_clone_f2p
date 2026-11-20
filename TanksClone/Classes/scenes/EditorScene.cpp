#include "scenes/EditorScene.h"

#include <algorithm>
#include <array>
#include <vector>
#include <cmath>
#include "ui/Background.h"
#include "game/EnemyCatalog.h"
#include "game/ResourceManifest.h"
#include "core/GameConstants.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/StringsManager.h"

namespace tanks::view::cocos
{

namespace
{
	struct PaletteItem
	{
		char Tile;
		const char* NameKey;
	};

	const std::array<PaletteItem, 8> Palette = { {
		{ tiles::Brick, "TILE_BRICK" },
		{ tiles::Stone, "TILE_STONE" },
		{ tiles::Wood, "TILE_WOOD" },
		{ tiles::PlayerSpawn, "TILE_PLAYER" },
		{ tiles::EnemySpawn, "TILE_ENEMY" },
		{ tiles::Ammo, "TILE_AMMO" },
		{ tiles::Repair, "TILE_REPAIR" },
		{ tiles::Empty, "EDITOR_ERASE" }
	} };
	constexpr size_t PaletteSize = std::tuple_size<decltype(Palette)>::value;

	constexpr float PaletteX = 30.0f;
	constexpr float PaletteStep = 46.0f;
	constexpr float PaletteButton = 40.0f;
	constexpr float MiniButton = 36.0f;
	constexpr float ActionWidth = 118.0f;
	constexpr float ActionHeight = 40.0f;
	constexpr int MinEnemies = 1;
	constexpr int MaxEnemies = 40;
	constexpr int MaxAlive = 4;
	constexpr int ZToolbar = 10;
	constexpr int ZMessage = 20;

	std::string Text(const std::string& InId)
	{
		return StringsManager::GetInstance().GetString(InId);
	}

	// A picture of a tile, sized for the preview (InScale 1) or for the map (InScale 2).
	cocos2d::Node* CreateTileNode(const char InTile, float InScale, const std::string& InTileset)
	{
		std::string CurrentFrame;
		switch ( InTile )
		{
			case tiles::Brick: CurrentFrame = ResourceManifest::GetTilesetFrame(InTileset, "wall_brick"); break;
			case tiles::Stone: CurrentFrame = ResourceManifest::GetTilesetFrame(InTileset, "wall_stone"); break;
			case tiles::Wood: CurrentFrame = ResourceManifest::GetTilesetFrame(InTileset, "wall_wood"); break;
			case tiles::PlayerSpawn: CurrentFrame = "player_tank"; break;
			case tiles::EnemySpawn: CurrentFrame = "spawn_marker"; break;
			case tiles::Ammo: CurrentFrame = "bonus_ammo"; InScale *= 1.25f; break;
			case tiles::Repair: CurrentFrame = "bonus_repair"; InScale *= 1.25f; break;
			default: return nullptr;
		}

		auto CurrentSprite = widgets::CreateFrameSprite(CurrentFrame);
		if ( CurrentSprite )
		{
			CurrentSprite->setScale(InScale);
		}
		return CurrentSprite;
	}
}

cocos2d::Scene* EditorScene::CreateScene()
{
	return EditorScene::create();
}

bool EditorScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}

	const ResourceManifest Manifest;
	Tileset = Manifest.GetTilesetName("");
	Atlases.Add(Manifest.GetTilesetAtlas(Tileset));

	if ( !LevelLoader::HasCustom() || !LevelLoader::LoadCustom(Level) )
	{
		Level = LevelLoader::CreateEmpty();
		Level.Rows[GridRows - 1][GridColumns / 2] = tiles::PlayerSpawn;
		Level.Rows[0][0] = tiles::EnemySpawn;
	}
	Level.EnemyIds.clear();
	for ( const auto& Enemy : EnemyCatalog::GetInstance().GetAll() )
	{
		Level.EnemyIds.push_back(Enemy.Id);
	}

	addChild(widgets::CreateScreenBackground(), -1);
	addChild(CreateBackground(7u, Tileset));

	Grid = Node::create();
	addChild(Grid, 1);
	CellNodes.assign(GridRows * GridColumns, nullptr);
	for ( int Row = 0; Row < GridRows; ++Row )
	{
		for ( int Column = 0; Column < GridColumns; ++Column )
		{
			RefreshCell(Row, Column);
		}
	}

	BuildToolbar();

	// Painting: click or drag over the map.
	auto CurrentTouch = cocos2d::EventListenerTouchOneByOne::create();
	CurrentTouch->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		PaintAt(InTouch->getLocation());
		return true;
	};
	CurrentTouch->onTouchMoved = [this](cocos2d::Touch* const InTouch, cocos2d::Event*) { PaintAt(InTouch->getLocation()); };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(CurrentTouch, Grid);

	auto Keyboard = cocos2d::EventListenerKeyboard::create();
	Keyboard->onKeyPressed = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event*)
	{
		if ( InKeyCode >= cocos2d::EventKeyboard::KeyCode::KEY_1 && InKeyCode <= cocos2d::EventKeyboard::KeyCode::KEY_8 )
		{
			SelectTile(static_cast<size_t>(InKeyCode) - static_cast<size_t>(cocos2d::EventKeyboard::KeyCode::KEY_1));
		}
		else if ( InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE )
		{
			SwitchScene(MenuScene::CreateScene());
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Keyboard, this);
	return true;
}

void EditorScene::BuildToolbar()
{
	auto Bar = cocos2d::DrawNode::create();
	Bar->drawSolidRect(cocos2d::Vec2(0, GridRows * TileSize), cocos2d::Vec2(DesignWidth, DesignHeight), cocos2d::Color4F(0.07f, 0.08f, 0.11f, 1.0f));
	Bar->drawSolidRect(cocos2d::Vec2(0, GridRows * TileSize), cocos2d::Vec2(DesignWidth, GridRows * TileSize + 2), cocos2d::Color4F(0.35f, 0.4f, 0.5f, 1.0f));
	addChild(Bar, ZToolbar - 1);

	const float Y = (GridRows * TileSize + DesignHeight) / 2; // middle of the toolbar

	// Palette
	for ( size_t Index = 0; Index < PaletteSize; ++Index )
	{
		auto CurrentButton = widgets::Button::create("", cocos2d::Size(PaletteButton, PaletteButton), [this, Index]() { SelectTile(Index); });
		CurrentButton->setPosition(PaletteX + Index * PaletteStep, Y);
		addChild(CurrentButton, ZToolbar);
		if ( auto Preview = CreateTileNode(Palette[Index].Tile, 1.0f, Tileset) )
		{
			Preview->setPosition(CurrentButton->getPosition());
			addChild(Preview, ZToolbar + 1);
		}
		else
		{
			// the eraser
			auto Cross = cocos2d::DrawNode::create();
			Cross->drawLine(cocos2d::Vec2(-10, -10), cocos2d::Vec2(10, 10), cocos2d::Color4F(0.9f, 0.35f, 0.3f, 1.0f));
			Cross->drawLine(cocos2d::Vec2(-10, 10), cocos2d::Vec2(10, -10), cocos2d::Color4F(0.9f, 0.35f, 0.3f, 1.0f));
			Cross->setPosition(CurrentButton->getPosition());
			addChild(Cross, ZToolbar + 1);
		}
		PaletteButtons.push_back(CurrentButton);
	}
	SelectTile(0);

	// Number of enemies
	const float CounterX = PaletteX + PaletteStep * PaletteSize + 24.0f;
	auto Minus = widgets::Button::create("-", cocos2d::Size(MiniButton, MiniButton), [this]() { ChangeEnemyCount(-1); });
	Minus->setPosition(CounterX, Y);
	addChild(Minus, ZToolbar);

	EnemyLabel = widgets::CreateLabel("", 22.0f, widgets::colors::TextNormal);
	EnemyLabel->setPosition(CounterX + 50.0f, Y);
	addChild(EnemyLabel, ZToolbar);

	auto Plus = widgets::Button::create("+", cocos2d::Size(MiniButton, MiniButton), [this]() { ChangeEnemyCount(1); });
	Plus->setPosition(CounterX + 100.0f, Y);
	addChild(Plus, ZToolbar);
	UpdateEnemyLabel();

	// Actions
	const cocos2d::Size ActionSize(ActionWidth, ActionHeight);
	float X = DesignWidth - 4 * (ActionWidth + 4) + ActionWidth / 2 + 4;
	const std::vector<std::pair<const char*, std::function<void()>>> Actions = {
		{ "EDITOR_SAVE", [this]() { Save(); } },
		{ "EDITOR_PLAY", [this]() { Test(); } },
		{ "EDITOR_CLEAR", [this]() { ResetMap(); } },
		{ "MENU_BACK", []() { SwitchScene(MenuScene::CreateScene()); } }
	};
	for ( const auto& ToolbarAction : Actions )
	{
		auto CurrentButton = widgets::Button::create(Text(ToolbarAction.first), ActionSize, ToolbarAction.second);
		CurrentButton->setPosition(X, Y);
		addChild(CurrentButton, ZToolbar);
		X += ActionWidth + 4;
	}
}

void EditorScene::SelectTile(const size_t InPaletteIndex)
{
	if ( InPaletteIndex >= PaletteSize )
	{
		return;
	}
	SelectedTile = InPaletteIndex;
	for ( size_t Index = 0; Index < PaletteButtons.size(); ++Index )
	{
		PaletteButtons[Index]->SetSelected(Index == SelectedTile);
	}
}

void EditorScene::PaintAt(const cocos2d::Vec2& InPoint)
{
	if ( InPoint.x < 0 || InPoint.y < 0 || InPoint.x >= GridColumns * TileSize || InPoint.y >= GridRows * TileSize )
	{
		return;
	}

	const int Column = static_cast<int>(InPoint.x / TileSize);
	const int Row = GridRows - 1 - static_cast<int>(InPoint.y / TileSize);
	SetCell(Row, Column, Palette[SelectedTile].Tile);
}

void EditorScene::SetCell(const int InRow, const int InColumn, const char InTile)
{
	if ( Level.Rows[InRow][InColumn] == InTile )
	{
		return;
	}

	// There is only one player start: moving it removes the old one.
	if ( InTile == tiles::PlayerSpawn )
	{
		for ( int Row = 0; Row < GridRows; ++Row )
		{
			for ( int Column = 0; Column < GridColumns; ++Column )
			{
				if ( Level.Rows[Row][Column] == tiles::PlayerSpawn )
				{
					Level.Rows[Row][Column] = tiles::Empty;
					RefreshCell(Row, Column);
				}
			}
		}
	}

	Level.Rows[InRow][InColumn] = InTile;
	RefreshCell(InRow, InColumn);
}

void EditorScene::RefreshCell(const int InRow, const int InColumn)
{
	Node*& Cell = CellNodes[InRow * GridColumns + InColumn];
	if ( Cell )
	{
		Cell->removeFromParent();
		Cell = nullptr;
	}

	Cell = CreateTileNode(Level.Rows[InRow][InColumn], 2.0f, Tileset);
	if ( Cell )
	{
		Cell->setPosition((InColumn + 0.5f) * TileSize, (GridRows - InRow - 0.5f) * TileSize);
		Grid->addChild(Cell);
	}
}

void EditorScene::ResetMap()
{
	const int Total = Level.TotalEnemies;
	Level = LevelLoader::CreateEmpty();
	Level.TotalEnemies = Total;
	Level.MaxAlive = std::min(Total, MaxAlive);
	Level.EnemyIds.clear();
	for ( const auto& Enemy : EnemyCatalog::GetInstance().GetAll() )
	{
		Level.EnemyIds.push_back(Enemy.Id);
	}
	Level.Rows[GridRows - 1][GridColumns / 2] = tiles::PlayerSpawn;
	Level.Rows[0][0] = tiles::EnemySpawn;

	for ( int Row = 0; Row < GridRows; ++Row )
	{
		for ( int Column = 0; Column < GridColumns; ++Column )
		{
			RefreshCell(Row, Column);
		}
	}
}

void EditorScene::ChangeEnemyCount(const int InDelta)
{
	Level.TotalEnemies = std::max(MinEnemies, std::min(MaxEnemies, Level.TotalEnemies + InDelta));
	Level.MaxAlive = std::min(Level.TotalEnemies, MaxAlive);
	UpdateEnemyLabel();
}

void EditorScene::UpdateEnemyLabel()
{
	EnemyLabel->setString(Text("HUD_ENEMIES") + " " + std::to_string(Level.TotalEnemies));
	EnemyLabel->setScale(std::min(1.0f, 70.0f / std::max(1.0f, EnemyLabel->getContentSize().width / 2.0f)));
}

bool EditorScene::CheckLevel()
{
	const std::string Problem = LevelLoader::Validate(Level);
	if ( !Problem.empty() )
	{
		ShowMessage(Text(Problem), false);
		return false;
	}
	return true;
}

void EditorScene::Save()
{
	if ( !CheckLevel() )
	{
		return;
	}
	ShowMessage(LevelLoader::SaveCustom(Level) ? Text("EDITOR_SAVED") : "Could not write the file", true);
}

void EditorScene::Test()
{
	if ( CheckLevel() && LevelLoader::SaveCustom(Level) )
	{
		SwitchScene(GameScene::CreateCustomScene(GameScene::Origin::Editor));
	}
}

void EditorScene::ShowMessage(const std::string& InText, const bool bInGood)
{
	auto CurrentLabel = widgets::CreateLabel(InText, 26.0f, bInGood ? widgets::colors::Good : widgets::colors::Bad);
	CurrentLabel->setPosition(DesignWidth / 2, 40.0f);
	CurrentLabel->enableOutline(cocos2d::Color4B(0, 0, 0, 255), 2);
	addChild(CurrentLabel, ZMessage);
	CurrentLabel->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(2.0f), cocos2d::FadeOut::create(0.4f), cocos2d::RemoveSelf::create(), nullptr));
}

} // namespace tanks::view::cocos
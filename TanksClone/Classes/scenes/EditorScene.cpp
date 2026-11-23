#include "scenes/EditorScene.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "ui/ActionListener.h"
#include "ui/CocosConvert.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/EnemyCatalog.h"
#include "game/ResourceManifest.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/ConfigRefs.h"
#include "services/LevelLegendManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/Background.h"

namespace tanks::view::cocos
{

namespace
{
	const viewmodel::EditorStyle& Style()
	{
		return GameContext::Get().GetManager<UiStyle>().GetEditor();
	}

	cocos2d::Color4F Paint(const model::Color& InColor)
	{
		return cocos2d::Color4F(ToCocos(InColor));
	}

	constexpr std::size_t ActionCount = 4;
	constexpr int ZPanel = 9;
	constexpr int ZToolbar = 10;
	constexpr int ZMessage = 20;


	std::string Text(const std::string& InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}

	void DrawPanel(cocos2d::DrawNode* const InCanvas, const model::RectF& InRect)
	{
		const cocos2d::Vec2 From = ToCocos(InRect.Origin);
		const cocos2d::Vec2 To = ToCocos(InRect.Origin + InRect.Size);
		InCanvas->drawSolidRect(From, To, Paint(Style().PanelColor));
		InCanvas->drawRect(From, To, Paint(Style().PanelBorder));
	}

	cocos2d::Vec2 CenterOf(const model::RectF& InRect)
	{
		return ToCocos(InRect.GetCenter());
	}

	cocos2d::Size SizeOf(const model::RectF& InRect)
	{
		return ToCocosSize(InRect.Size);
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
	Atlases.AddAtlas(Manifest.GetTilesetAtlas(Tileset));
	LoadPalette();

	if ( !LevelLoader::HasCustom() || !LevelLoader::LoadCustom(Level) )
	{
		Level = LevelLoader::CreateEmpty();
		Level.Rows[GridRows - 1][GridColumns / 2] = tiles::PlayerSpawn;
		Level.Rows[0][0] = tiles::EnemySpawn;
	}
	Level.EnemyIds.clear();
	for ( const auto& Enemy : GameContext::Get().GetManager<EnemyCatalog>().GetAll() )
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

	BuildPanels();

	auto CurrentTouch = cocos2d::EventListenerTouchOneByOne::create();
	CurrentTouch->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		PaintAt(Grid->convertToNodeSpace(InTouch->getLocation()));
		return true;
	};
	CurrentTouch->onTouchMoved = [this](cocos2d::Touch* const InTouch, cocos2d::Event*) { PaintAt(Grid->convertToNodeSpace(InTouch->getLocation())); };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(CurrentTouch, Grid);

	AddActionHandler(this, [this](const model::InputAction InAction)
	{
		if ( InAction >= model::InputAction::SelectItem1 && InAction <= model::InputAction::SelectItem9 )
		{
			SelectItem(static_cast<std::size_t>(InAction) - static_cast<std::size_t>(model::InputAction::SelectItem1));
		}
		else if ( InAction == model::InputAction::NextTab && !Palette.GetTabs().empty() )
		{
			SelectTab((ActiveTab + 1) % Palette.GetTabs().size());
		}
		else if ( InAction == model::InputAction::Back )
		{
			SwitchScene(MenuScene::CreateScene());
		}
	}, model::InputContext::Editor);
	return true;
}

void EditorScene::LoadPalette()
{
	const ConfigReader& Reader = GameContext::Get().GetManager<ConfigReader>();
	ResourceManager& Resources = GameContext::Get().GetManager<ResourceManager>();
	const auto FindConfig = [&Reader, &Resources](const std::string& InId) -> const model::IConfigNode*
	{
		const std::string Path = Resources.ResolvePath(model::ResourceId(InId), model::ResourceKind::Config);
		return Reader.HasConfig(Path) ? &Reader.GetConfig(Path) : nullptr;
	};
	if ( !Palette.FromConfig(Reader.GetConfig(config_refs::ResolveId(ids::configs::EditorPalette)), GameContext::Get().GetManager<LevelLegendManager>().Get(), FindConfig) )
	{
		cocos2d::log("Editor palette is empty");
	}
	for ( const std::string& Error : Palette.GetErrors() )
	{
		cocos2d::log("Editor palette: %s", Error.c_str());
	}
}

char EditorScene::GetSelectedTile() const
{
	const auto& Entries = Palette.GetEntries();
	return SelectedEntry < Entries.size() ? Entries[SelectedEntry].Tile : tiles::Empty;
}

model::RectF EditorScene::GetToolbarBounds() const
{
	const model::RectF Visible = GetScreenLayout().GetVisibleRect();
	const float MapTop = GridRows * TileSize;
	return model::RectF(Visible.GetMinX(), MapTop, Visible.Size.X, std::max(0.0f, Visible.GetMaxY() - MapTop));
}

void EditorScene::BuildPanels()
{
	const model::RectF Bounds = GetToolbarBounds();
	const model::EditorLayoutResult Layout = model::EditorLayout(ActionCount, Palette.GetTabs().size(), Style().Layout).Compute(Bounds, 0);

	auto Canvas = cocos2d::DrawNode::create();
	Canvas->drawSolidRect(ToCocos(Bounds.Origin), ToCocos(Bounds.Origin + Bounds.Size), Paint(Style().ToolbarColor));
	Canvas->drawSolidRect(ToCocos(Bounds.Origin), cocos2d::Vec2(Bounds.GetMaxX(), Bounds.GetMinY() + 2), Paint(Style().PanelBorder));
	DrawPanel(Canvas, Layout.PalettePanel);
	DrawPanel(Canvas, Layout.ActionPanel);
	addChild(Canvas, ZPanel);

	ItemsLayer = Node::create();
	addChild(ItemsLayer, ZToolbar);

	BuildActions(Layout);
	BuildTabs(Layout);
	SelectTab(0);
}

void EditorScene::BuildActions(const model::EditorLayoutResult& InLayout)
{
	const std::vector<std::pair<const char*, std::function<void()>>> Actions = {
		{ "EDITOR_SAVE", [this]() { Save(); } },
		{ "EDITOR_PLAY", [this]() { Test(); } },
		{ "EDITOR_CLEAR", [this]() { ResetMap(); } },
		{ "MENU_BACK", []() { SwitchScene(MenuScene::CreateScene()); } }
	};
	for ( std::size_t Index = 0; Index < Actions.size() && Index < InLayout.Actions.size(); ++Index )
	{
		auto CurrentButton = widgets::Button::create(Text(Actions[Index].first), SizeOf(InLayout.Actions[Index]), Actions[Index].second);
		CurrentButton->setPosition(CenterOf(InLayout.Actions[Index]));
		addChild(CurrentButton, ZToolbar);
	}

	auto Minus = widgets::Button::create("-", SizeOf(InLayout.CounterMinus), [this]() { ChangeEnemyCount(-1); });
	Minus->setPosition(CenterOf(InLayout.CounterMinus));
	addChild(Minus, ZToolbar);

	auto Plus = widgets::Button::create("+", SizeOf(InLayout.CounterPlus), [this]() { ChangeEnemyCount(1); });
	Plus->setPosition(CenterOf(InLayout.CounterPlus));
	addChild(Plus, ZToolbar);

	EnemyLabelBox = SizeOf(InLayout.CounterLabel);
	EnemyLabel = widgets::CreateLabel("", Style().EnemyLabelFontSize, widgets::colors::TextNormal());
	EnemyLabel->setPosition(CenterOf(InLayout.CounterLabel));
	addChild(EnemyLabel, ZToolbar);
	UpdateEnemyLabel();
}

void EditorScene::BuildTabs(const model::EditorLayoutResult& InLayout)
{
	const auto& Tabs = Palette.GetTabs();
	for ( std::size_t Index = 0; Index < Tabs.size() && Index < InLayout.Tabs.size(); ++Index )
	{
		auto CurrentButton = widgets::Button::create(Text(Tabs[Index].NameKey), SizeOf(InLayout.Tabs[Index]), [this, Index]() { SelectTab(Index); });
		CurrentButton->setPosition(CenterOf(InLayout.Tabs[Index]));
		addChild(CurrentButton, ZToolbar);
		TabButtons.push_back(CurrentButton);
	}
}

void EditorScene::SelectTab(const std::size_t InTab)
{
	if ( InTab >= Palette.GetTabs().size() )
	{
		return;
	}
	ActiveTab = InTab;
	for ( std::size_t Index = 0; Index < TabButtons.size(); ++Index )
	{
		TabButtons[Index]->SetSelected(Index == ActiveTab);
	}
	TabEntries = Palette.GetEntriesOfTab(ActiveTab);
	BuildItems();
	SelectItem(0);
}

void EditorScene::BuildItems()
{
	ItemsLayer->removeAllChildren();
	ItemButtons.clear();

	const model::EditorLayoutResult Layout = model::EditorLayout(ActionCount, Palette.GetTabs().size(), Style().Layout).Compute(GetToolbarBounds(), TabEntries.size());
	for ( std::size_t Index = 0; Index < TabEntries.size() && Index < Layout.Items.size(); ++Index )
	{
		const model::RectF& Box = Layout.Items[Index];
		auto CurrentButton = widgets::Button::create("", SizeOf(Box), [this, Index]() { SelectItem(Index); });
		CurrentButton->setPosition(CenterOf(Box));
		ItemsLayer->addChild(CurrentButton);
		if ( auto Icon = CreateIcon(Palette.GetEntries()[TabEntries[Index]], SizeOf(Box)) )
		{
			Icon->setPosition(CenterOf(Box));
			ItemsLayer->addChild(Icon, 1);
		}
		ItemButtons.push_back(CurrentButton);
	}
}

void EditorScene::SelectItem(const std::size_t InTabItem)
{
	if ( InTabItem >= TabEntries.size() )
	{
		return;
	}
	SelectedEntry = TabEntries[InTabItem];
	for ( std::size_t Index = 0; Index < ItemButtons.size(); ++Index )
	{
		ItemButtons[Index]->SetSelected(Index == InTabItem);
	}
}

cocos2d::Node* EditorScene::CreateIcon(const model::EditorPaletteEntry& InEntry, const cocos2d::Size& InBox) const
{
	const float Half = std::min(InBox.width, InBox.height) * Style().IconFill / 2;
	if ( InEntry.Icon == model::PaletteIcon::Erase )
	{
		auto Cross = cocos2d::DrawNode::create();
		Cross->drawLine(cocos2d::Vec2(-Half, -Half), cocos2d::Vec2(Half, Half), Paint(Style().EraseColor));
		Cross->drawLine(cocos2d::Vec2(-Half, Half), cocos2d::Vec2(Half, -Half), Paint(Style().EraseColor));
		return Cross;
	}

	const std::string Frame = InEntry.Icon == model::PaletteIcon::TilesetFrame ? ResourceManifest::GetTilesetFrame(Tileset, InEntry.Frame) : InEntry.Frame;
	auto Icon = widgets::CreateFrameSprite(Frame);
	if ( Icon )
	{
		const cocos2d::Size Content = Icon->getContentSize();
		Icon->setScale(2 * Half / std::max(1.0f, std::max(Content.width, Content.height)));
	}
	return Icon;
}

cocos2d::Node* EditorScene::CreateTileNode(const char InTile) const
{
	const model::EditorPaletteEntry* const Entry = Palette.FindByTile(InTile);
	if ( !Entry || Entry->Icon == model::PaletteIcon::Erase )
	{
		return nullptr;
	}

	const std::string Frame = Entry->Icon == model::PaletteIcon::TilesetFrame ? ResourceManifest::GetTilesetFrame(Tileset, Entry->Frame) : Entry->Frame;
	auto Tile = widgets::CreateFrameSprite(Frame);
	if ( Tile )
	{
		Tile->setScale(Entry->MapScale);
	}
	return Tile;
}

void EditorScene::PaintAt(const cocos2d::Vec2& InPoint)
{
	if ( InPoint.x < 0 || InPoint.y < 0 || InPoint.x >= GridColumns * TileSize || InPoint.y >= GridRows * TileSize )
	{
		return;
	}

	const int Column = static_cast<int>(InPoint.x / TileSize);
	const int Row = GridRows - 1 - static_cast<int>(InPoint.y / TileSize);
	SetCell(Row, Column, GetSelectedTile());
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

	Cell = CreateTileNode(Level.Rows[InRow][InColumn]);
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
	Level.MaxAlive = std::min(Total, Style().MaxAlive);
	Level.EnemyIds.clear();
	for ( const auto& Enemy : GameContext::Get().GetManager<EnemyCatalog>().GetAll() )
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
	Level.TotalEnemies = std::max(Style().MinEnemies, std::min(Style().MaxEnemies, Level.TotalEnemies + InDelta));
	Level.MaxAlive = std::min(Level.TotalEnemies, Style().MaxAlive);
	UpdateEnemyLabel();
}

void EditorScene::UpdateEnemyLabel()
{
	EnemyLabel->setScale(1.0f);
	EnemyLabel->setString(Text("HUD_ENEMIES") + " " + std::to_string(Level.TotalEnemies));
	const cocos2d::Size Content = EnemyLabel->getContentSize();
	const float WidthFit = EnemyLabelBox.width / std::max(1.0f, Content.width);
	const float HeightFit = EnemyLabelBox.height / std::max(1.0f, Content.height);
	EnemyLabel->setScale(std::min(1.0f, std::min(WidthFit, HeightFit)));
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
	const viewmodel::WidgetStyle& Toast = GameContext::Get().GetManager<UiStyle>().GetWidgets();
	auto CurrentLabel = widgets::CreateLabel(InText, Toast.ToastFontSize, bInGood ? widgets::colors::Good() : widgets::colors::Bad());
	CurrentLabel->setPosition(DesignWidth / 2, Style().MessageBottom);
	widgets::AddOutline(CurrentLabel);
	addChild(CurrentLabel, ZMessage);
	CurrentLabel->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(Toast.ToastShowTime), cocos2d::FadeOut::create(Toast.ToastFadeTime),
		cocos2d::RemoveSelf::create(), nullptr));
}

} // namespace tanks::view::cocos

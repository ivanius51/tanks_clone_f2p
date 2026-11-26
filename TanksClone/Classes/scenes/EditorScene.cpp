#include "scenes/EditorScene.h"
#include "model/Log.h"
#include "templates/TFunction.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <utility>

#include "ui/ActionListener.h"
#include "ui/CocosConvert.h"

#include "core/BuildInfo.h"
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
#include "services/HoverManager.h"
#include "services/LevelLegendManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/Background.h"
#include "ui/WidgetFactory.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{
	// getLocation() flips Y again; the desktop view already sends GL coordinates.
	cocos2d::Vec2 MouseWorld(const cocos2d::EventMouse& InEvent)
	{
		return cocos2d::Vec2(InEvent.getCursorX(), InEvent.getCursorY());
	}


namespace
{
	const viewmodel::EditorStyle& Style()
	{
		return GameContext::Get<UiStyle>().GetEditor();
	}

	cocos2d::Color4F Paint(const model::Color& InColor)
	{
		return cocos2d::Color4F(ToCocos(InColor));
	}

	constexpr int ZMap = 1;
	constexpr int ZMapChrome = 3;
	constexpr int ZDocks = 5;
	constexpr int ZMessage = 20;
	constexpr int ZMenu = 30;
	constexpr int CellMargin = 1;
	constexpr int MaxMinimapSamples = 96;
	constexpr std::size_t MaxHistoryRows = 9;
	constexpr float SelectedRowOpacity = 0.8f;
	constexpr float FutureRowOpacity = 0.45f;
	constexpr std::string_view GroupMain = "main";
	constexpr std::string_view GroupTest = "test";
	constexpr std::string_view DockStripId = "dock_strip";
	constexpr std::string_view ShowEnemiesKey = "total_enemies";

	static_assert(model::EditorDocument::EmptyTile == tiles::Empty);

	std::string Text(const std::string& InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
	}

	model::RectF GetScreenBounds()
	{
		return GetScreenLayout().GetVisibleRect();
	}

	long long CellKey(const model::EditorCell& InCell)
	{
		return static_cast<long long>(InCell.Row) * 1000003LL + InCell.Column;
	}

	model::EditorCell CellOfKey(const long long InKey)
	{
		return model::EditorCell{ static_cast<int>(InKey / 1000003LL), static_cast<int>(InKey % 1000003LL) };
	}

	void SetRow(LayoutWidget* const InRow, const std::string& InText, const std::string& InValue, const bool bInSelected, const LayoutWidget::ClickAction& InAction)
	{
		InRow->SetText("text", InText);
		InRow->SetText("value", InValue);
		InRow->SetPartOpacity("background", bInSelected ? SelectedRowOpacity : 0.0f);
		InRow->SetClickHandler(InAction);
	}

	void SetStepper(LayoutWidget* const InOwner, const std::string_view InId, const std::string& InValue, const LayoutWidget::ClickAction& InPrevious, const LayoutWidget::ClickAction& InNext)
	{
		LayoutWidget* const Stepper = InOwner->FindWidget(InId);
		Stepper->SetText("value", InValue);
		Stepper->BindClick("prev", InPrevious);
		Stepper->BindClick("next", InNext);
	}

	model::Color TileColor(const char InTile)
	{
		const model::LegendEntry* const Entry = GameContext::Get<LevelLegendManager>().Get().Find(InTile);
		if ( !Entry )
		{
			return model::Color(200, 200, 200);
		}
		switch ( Entry->Role )
		{
		case model::TileRole::Wall:
			return model::Color(150, 150, 165);
		case model::TileRole::Player:
			return model::Color(110, 220, 110);
		case model::TileRole::Spawner:
			return model::Color(235, 90, 80);
		case model::TileRole::Pickup:
		case model::TileRole::Crate:
			return model::Color(255, 205, 70);
		default:
			return model::Color(200, 140, 90);
		}
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
	Library.Load(GameContext::Get().GetStorage());

	if ( !LevelLoader::HasCustom() || !LevelLoader::LoadCustom(Level) )
	{
		Level = LevelLoader::CreateEmpty();
		Level.Rows[Level.Rows.Size() - 1][Level.Rows.Front().size() / 2] = tiles::PlayerSpawn;
		Level.Rows[0][0] = tiles::EnemySpawn;
	}
	if ( !LevelLoader::HasOwnPools(Level) )
	{
		Level.EnemyIds.Clear();
		for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
		{
			Level.EnemyIds.PushBack(Enemy.Id);
		}
	}

	Document = model::EditorDocument(Level.Rows, CollectUniqueTiles());
	Document.SetProperty(std::string(ShowEnemiesKey), std::to_string(Level.TotalEnemies));
	ShownRows = Document.GetRows();

	addChild(widgets::CreateScreenBackground(), -1);
	BuildMap();
	DocksLayer = cocos2d::Node::create();
	addChild(DocksLayer, ZDocks);
	SelectTab(0);
	BuildDocks();
	ApplyLayout();
	MenuLayer = cocos2d::Node::create();
	addChild(MenuLayer, ZMenu);
	Viewport.CenterOn(model::Vec2f(Viewport.GetContentSize().X / 2, Viewport.GetContentSize().Y / 2));
	ApplyScroll();

	auto CurrentTouch = cocos2d::EventListenerTouchOneByOne::create();
	CurrentTouch->setSwallowTouches(true);
	CurrentTouch->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*) { return OnTouchBegan(ToLayoutSpace(InTouch->getLocation())); };
	CurrentTouch->onTouchMoved = [this](cocos2d::Touch* const InTouch, cocos2d::Event*) { OnTouchMoved(ToLayoutSpace(InTouch->getLocation())); };
	CurrentTouch->onTouchEnded = [this](cocos2d::Touch*, cocos2d::Event*) { OnTouchEnded(); };
	CurrentTouch->onTouchCancelled = [this](cocos2d::Touch*, cocos2d::Event*) { OnTouchEnded(); };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(CurrentTouch, MapClip);

	auto CurrentMouse = cocos2d::EventListenerMouse::create();
	CurrentMouse->onMouseScroll = [this](cocos2d::Event* const InEvent)
	{
		const auto Mouse = static_cast<cocos2d::EventMouse*>(InEvent);
		if ( !Layout.MapView.Contains(ToLayoutSpace(MouseWorld(*Mouse))) )
		{
			return;
		}
		ScrollMap(model::Vec2f(Mouse->getScrollX() * GetTileSize(), -Mouse->getScrollY() * GetTileSize()));
	};
	CurrentMouse->onMouseDown = [this](cocos2d::Event* const InEvent)
	{
		const auto Mouse = static_cast<cocos2d::EventMouse*>(InEvent);
		if ( Mouse->getMouseButton() == cocos2d::EventMouse::MouseButton::BUTTON_RIGHT && Layout.MapView.Contains(ToLayoutSpace(MouseWorld(*Mouse))) )
		{
			OpenMenu(ToLayoutSpace(MouseWorld(*Mouse)));
		}
		else if ( Mouse->getMouseButton() == cocos2d::EventMouse::MouseButton::BUTTON_MIDDLE && Layout.MapView.Contains(ToLayoutSpace(MouseWorld(*Mouse))) )
		{
			bPanning = true;
			PanFrom = ToLayoutSpace(MouseWorld(*Mouse));
		}
	};
	CurrentMouse->onMouseMove = [this](cocos2d::Event* const InEvent)
	{
		if ( !bPanning )
		{
			return;
		}
		const model::Vec2f Now = ToLayoutSpace(MouseWorld(*static_cast<cocos2d::EventMouse*>(InEvent)));
		ScrollMap(PanFrom - Now);
		PanFrom = Now;
	};
	CurrentMouse->onMouseUp = [this](cocos2d::Event* const InEvent)
	{
		if ( static_cast<cocos2d::EventMouse*>(InEvent)->getMouseButton() == cocos2d::EventMouse::MouseButton::BUTTON_MIDDLE )
		{
			bPanning = false;
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(CurrentMouse, this);

	AddActionHandler(this, [this](const model::InputAction InAction)
	{
		if ( InAction >= model::InputAction::SelectItem1 && InAction <= model::InputAction::SelectItem9 )
		{
			SelectItem(static_cast<std::size_t>(InAction) - static_cast<std::size_t>(model::InputAction::SelectItem1));
		}
		else if ( InAction == model::InputAction::NextTab && !Palette.GetTabs().IsEmpty() )
		{
			SelectTab((ActiveTab + 1) % Palette.GetTabs().Size());
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
	const ConfigReader& Reader = GameContext::Get<ConfigReader>();
	ResourceManager& Resources = GameContext::Get<ResourceManager>();
	const auto FindConfig = [&Reader, &Resources](const std::string& InId) -> const model::IConfigNode*
	{
		const FPersistentString Path = Resources.ResolvePath(model::ResourceId(InId), model::ResourceKind::Config);
		return Reader.HasConfig(Path) ? &Reader.GetConfig(Path) : nullptr;
	};
	if ( !Palette.FromConfig(Reader.GetConfig(config_refs::ResolveId(ids::configs::EditorPalette)), GameContext::Get<LevelLegendManager>().Get(), FindConfig) )
	{
		model::Log::Info("Editor palette is empty");
	}
	for ( const std::string& Error : Palette.GetErrors() )
	{
		model::Log::Info("Editor palette: %s", Error.c_str());
	}
}

char EditorScene::GetSelectedTile() const
{
	const auto& Entries = Palette.GetEntries();
	return SelectedEntry < Entries.Size() ? Entries[SelectedEntry].Tile : tiles::Empty;
}

model::Vec2f EditorScene::ToLayoutSpace(const cocos2d::Vec2& InWorldPoint) const
{
	return FromCocos(convertToNodeSpace(InWorldPoint));
}

void EditorScene::BuildMap()
{
	MapClip = cocos2d::ClippingRectangleNode::create();
	addChild(MapClip, ZMap);

	auto Ground = cocos2d::DrawNode::create();
	MapClip->addChild(Ground, -1);
	MapGround = Ground;

	MapContent = cocos2d::Node::create();
	MapClip->addChild(MapContent);

	MinimapTiles = cocos2d::DrawNode::create();
	addChild(MinimapTiles, ZMapChrome - 1);
	MapChrome = cocos2d::DrawNode::create();
	addChild(MapChrome, ZMapChrome);
}

void EditorScene::ApplyLayout()
{
	Layout = model::EditorLayout(Style().Layout).Compute(GetScreenBounds(), Docks);
	if ( bAutoZoom )
	{
		Zoom = ComputeFitZoom();
	}
	Viewport.SetViewSize(Layout.MapView.Size);
	Viewport.SetContentSize(model::Vec2f(Document.GetColumnCount() * GetTileSize(), Document.GetRowCount() * GetTileSize()));
	MapClip->setClippingRegion(ToCocos(Layout.MapView));

	MapGround->clear();
	MapGround->drawSolidRect(ToCocos(Layout.MapView.Origin), ToCocos(Layout.MapView.Origin + Layout.MapView.Size), Paint(model::Color(12, 14, 20)));

	MapContent->removeAllChildren();
	CellNodes.Clear();
	auto MapSurface = cocos2d::DrawNode::create();
	const float Width = Viewport.GetContentSize().X;
	const float Height = Viewport.GetContentSize().Y;
	MapSurface->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(Width, Height), Paint(model::Color(24, 28, 38)));
	for ( int Column = 0; Column <= Document.GetColumnCount(); ++Column )
	{
		MapSurface->drawLine(cocos2d::Vec2(Column * GetTileSize(), 0), cocos2d::Vec2(Column * GetTileSize(), Height), Paint(model::Color(255, 255, 255, 14)));
	}
	for ( int Row = 0; Row <= Document.GetRowCount(); ++Row )
	{
		MapSurface->drawLine(cocos2d::Vec2(0, Row * GetTileSize()), cocos2d::Vec2(Width, Row * GetTileSize()), Paint(model::Color(255, 255, 255, 14)));
	}
	MapSurface->drawRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(Width, Height), Paint(Style().PanelBorder));
	MapContent->addChild(MapSurface, -1);
	SelectionMark = cocos2d::DrawNode::create();
	MapContent->addChild(SelectionMark, 2);
	UpdateSelectionMark();

	ApplyScroll();
	RedrawMinimapTiles();
}

void EditorScene::ApplyScroll()
{
	const model::Vec2f Scroll = Viewport.GetScroll();
	MapContent->setPosition(Layout.MapView.GetMinX() - Scroll.X, Layout.MapView.GetMinY() - Scroll.Y);
	UpdateVisibleCells();
	RedrawMapChrome();
}

void EditorScene::ScrollMap(const model::Vec2f& InDelta)
{
	Viewport.ScrollBy(InDelta);
	ApplyScroll();
}

void EditorScene::UpdateVisibleCells()
{
	const model::Vec2f Scroll = Viewport.GetScroll();
	const model::Vec2f View = Viewport.GetViewSize();
	const int Rows = Document.GetRowCount();
	const int Columns = Document.GetColumnCount();
	const int FirstColumn = std::max(0, static_cast<int>(std::floor(Scroll.X / GetTileSize())) - CellMargin);
	const int LastColumn = std::min(Columns - 1, static_cast<int>(std::floor((Scroll.X + View.X) / GetTileSize())) + CellMargin);
	const int FirstRow = std::max(0, Rows - 1 - static_cast<int>(std::floor((Scroll.Y + View.Y) / GetTileSize())) - CellMargin);
	const int LastRow = std::min(Rows - 1, Rows - 1 - static_cast<int>(std::floor(Scroll.Y / GetTileSize())) + CellMargin);

	for ( auto Found = CellNodes.begin(); Found != CellNodes.end(); )
	{
		const model::EditorCell Cell = CellOfKey(Found->Key);
		if ( Cell.Row < FirstRow || Cell.Row > LastRow || Cell.Column < FirstColumn || Cell.Column > LastColumn )
		{
			Found->Value->removeFromParent();
			Found = CellNodes.Erase(Found);
		}
		else
		{
			++Found;
		}
	}
	for ( int Row = FirstRow; Row <= LastRow; ++Row )
	{
		for ( int Column = FirstColumn; Column <= LastColumn; ++Column )
		{
			const model::EditorCell Cell{ Row, Column };
			if ( CellNodes.Find(CellKey(Cell)) == nullptr )
			{
				RefreshCell(Cell);
			}
		}
	}
}

void EditorScene::RefreshCell(const model::EditorCell& InCell)
{
	auto* const Found = CellNodes.Find(CellKey(InCell));
	if ( Found )
	{
		(*Found)->removeFromParent();
		CellNodes.Remove(CellKey(InCell));
	}

	cocos2d::Node* const Tile = CreateTileNode(Document.GetTile(InCell));
	if ( !Tile )
	{
		return;
	}
	const model::Vec2f Centre = Viewport.CellCenter(InCell, GetTileSize());
	Tile->setPosition(Centre.X, Centre.Y);
	Tile->setScale(Tile->getScale() * Zoom);
	MapContent->addChild(Tile);
	CellNodes[CellKey(InCell)] = Tile;
}

void EditorScene::SyncCells()
{
	const TArray<std::string>& Rows = Document.GetRows();
	for ( int Row = 0; Row < Document.GetRowCount(); ++Row )
	{
		if ( Rows[Row] == ShownRows[Row] )
		{
			continue;
		}
		for ( int Column = 0; Column < Document.GetColumnCount(); ++Column )
		{
			if ( Rows[Row][Column] != ShownRows[Row][Column] )
			{
				RefreshCell(model::EditorCell{ Row, Column });
			}
		}
	}
	ShownRows = Rows;
	UpdateVisibleCells();
	RedrawMinimapTiles();
}

void EditorScene::RedrawMapChrome()
{
	MapChrome->clear();
	const auto Fill = [this](const model::RectF& InRect, const model::Color& InColor)
	{
		MapChrome->drawSolidRect(ToCocos(InRect.Origin), ToCocos(InRect.Origin + InRect.Size), Paint(InColor));
	};
	Fill(Layout.HorizontalBar, Style().PanelColor);
	Fill(Layout.VerticalBar, Style().PanelColor);
	if ( Viewport.CanScrollX() )
	{
		Fill(Viewport.GetHorizontalThumb(Layout.HorizontalBar), Style().PanelBorder);
	}
	if ( Viewport.CanScrollY() )
	{
		Fill(Viewport.GetVerticalThumb(Layout.VerticalBar), Style().PanelBorder);
	}

	const model::RectF& Minimap = Layout.Minimap;
	if ( Minimap.Size.X > 0.0f && (Viewport.CanScrollX() || Viewport.CanScrollY()) )
	{
		const model::RectF Shown = Viewport.GetMinimapViewRect(Minimap);
		MapChrome->drawRect(ToCocos(Shown.Origin), ToCocos(Shown.Origin + Shown.Size), Paint(model::Color(255, 205, 70)));
		MapChrome->drawRect(ToCocos(Minimap.Origin), ToCocos(Minimap.Origin + Minimap.Size), Paint(Style().PanelBorder));
	}
	DrawFloorTint();
	DrawObjectMarkers();
	DrawPointMarkers();
}

void EditorScene::RedrawMinimapTiles()
{
	MinimapTiles->clear();
	const model::RectF& Minimap = Layout.Minimap;
	if ( Minimap.Size.X <= 0.0f || !(Viewport.CanScrollX() || Viewport.CanScrollY()) || Document.GetColumnCount() == 0 )
	{
		return;
	}
	MinimapTiles->drawSolidRect(ToCocos(Minimap.Origin), ToCocos(Minimap.Origin + Minimap.Size), Paint(model::Color(10, 12, 18, 220)));

	const model::RectF Content = Viewport.GetMinimapContentRect(Minimap);
	const int Rows = Document.GetRowCount();
	const int Columns = Document.GetColumnCount();
	const int StepRow = std::max(1, (Rows + MaxMinimapSamples - 1) / MaxMinimapSamples);
	const int StepColumn = std::max(1, (Columns + MaxMinimapSamples - 1) / MaxMinimapSamples);
	const float CellWidth = Content.Size.X / Columns;
	const float CellHeight = Content.Size.Y / Rows;
	for ( int Row = 0; Row < Rows; Row += StepRow )
	{
		for ( int Column = 0; Column < Columns; Column += StepColumn )
		{
			const char Tile = Document.GetTile({ Row, Column });
			if ( Tile == tiles::Empty )
			{
				continue;
			}
			const float X = Content.GetMinX() + Column * CellWidth;
			const float Y = Content.GetMaxY() - (Row + StepRow) * CellHeight;
			MinimapTiles->drawSolidRect(cocos2d::Vec2(X, Y), cocos2d::Vec2(X + std::max(1.0f, CellWidth * StepColumn), Y + std::max(1.0f, CellHeight * StepRow)),
				Paint(TileColor(Tile)));
		}
	}
}

cocos2d::Node* EditorScene::CreateTileNode(const char InTile) const
{
	const model::EditorPaletteEntry* const Entry = Palette.FindByTile(InTile);
	if ( !Entry || Entry->Icon == model::PaletteIcon::Erase )
	{
		return nullptr;
	}

	auto Tile = widgets::CreateFrameSprite(GetEntryFrame(*Entry));
	if ( Tile )
	{
		Tile->setScale(Entry->MapScale);
	}
	return Tile;
}

std::string EditorScene::GetEntryFrame(const model::EditorPaletteEntry& InEntry) const
{
	return InEntry.Icon == model::PaletteIcon::TilesetFrame ? ResourceManifest::GetTilesetFrame(Tileset, InEntry.Frame) : InEntry.Frame;
}

cocos2d::Vec2 EditorScene::LayoutToWorld(const model::Vec2f& InPoint) const
{
	return convertToWorldSpace(ToCocos(InPoint));
}

cocos2d::Vec2 EditorScene::GetWorldCentre(const cocos2d::Node* const InNode) const
{
	return InNode->getParent()->convertToWorldSpace(InNode->getPosition());
}

cocos2d::Vec2 EditorScene::CellToWorld(const model::EditorCell& InCell) const
{
	const model::Vec2f InView = Viewport.ContentToView(Viewport.CellCenter(InCell, GetTileSize()));
	return LayoutToWorld(InView + Layout.MapView.Origin);
}

LayoutWidget* EditorScene::GetDockTab(const int InDock, const std::size_t InIndex) const
{
	return InIndex < DockTabs[InDock].Size() ? DockTabs[InDock][InIndex] : nullptr;
}

LayoutWidget* EditorScene::GetMenuLine(const model::EditorMenuAction InAction) const
{
	for ( const auto& [Action, Line] : MenuLines )
	{
		if ( Action == InAction )
		{
			return Line;
		}
	}
	return nullptr;
}

LayoutWidget* EditorScene::GetHistoryRow(const std::size_t InStep) const
{
	const std::size_t Visible = HistoryRows.Size();
	const std::size_t First = History.GetStep() + 1 > Visible ? History.GetStep() + 1 - Visible : 0;
	return InStep >= First && InStep - First < Visible ? HistoryRows[InStep - First] : nullptr;
}

LayoutWidget* EditorScene::GetLibraryRow(const std::size_t InIndex) const
{
	return InIndex < LibraryRows.Size() ? LibraryRows[InIndex] : nullptr;
}

bool EditorScene::IsDockCollapsed(const int InDock) const
{
	return InDock == LeftDock ? Layout.bLeftCollapsed : InDock == RightDock ? Layout.bRightCollapsed : Layout.bBottomCollapsed;
}

LayoutWidget* EditorScene::GetEnemyStepperButton(const bool bInPlus) const
{
	return EnemyStepper ? EnemyStepper->FindWidget(bInPlus ? "next" : "prev") : nullptr;
}

float EditorScene::GetTileSize() const
{
	return game::TileSize * Zoom;
}

void EditorScene::SetZoom(const float InZoom)
{
	bAutoZoom = false;
	Zoom = std::clamp(InZoom, Style().MinZoom, Style().MaxZoom);
	ShownRows.Clear();
	ApplyLayout();
	SyncCells();
}

float EditorScene::ComputeFitZoom() const
{
	if ( Document.GetColumnCount() == 0 || Document.GetRowCount() == 0 )
	{
		return Zoom;
	}
	const model::Vec2f View = Layout.MapView.Size;
	const float Fit = std::min(View.X / (Document.GetColumnCount() * game::TileSize), View.Y / (Document.GetRowCount() * game::TileSize));
	return std::clamp(Fit, Style().MinZoom, Style().MaxZoom);
}

void EditorScene::FitMap()
{
	SetZoom(ComputeFitZoom());
	bAutoZoom = true;
}

int EditorScene::GetEnemyCount() const
{
	return std::atoi(Document.GetProperty(std::string(ShowEnemiesKey)).c_str());
}

const model::RectF& EditorScene::GetDockRect(const int InDock) const
{
	return InDock == LeftDock ? Layout.LeftDock : InDock == RightDock ? Layout.RightDock : Layout.BottomDock;
}

model::RectF EditorScene::GetLocalBox(const cocos2d::Node* const InNode) const
{
	const cocos2d::Vec2 Centre = DocksLayer->convertToNodeSpace(InNode->getParent()->convertToWorldSpace(InNode->getPosition()));
	const cocos2d::Size Size = InNode->getContentSize();
	return model::RectF(model::Vec2f(Centre.x - Size.width / 2, Centre.y - Size.height / 2), model::Vec2f(Size.width, Size.height));
}

void EditorScene::SetAutoCollapse(const bool bInEnabled)
{
	Docks.bAutoCollapse = bInEnabled;
	BuildDocks();
}

void EditorScene::SetAllDocksCollapsed(const bool bInCollapsed)
{
	Docks.bLeftCollapsed = Docks.bRightCollapsed = Docks.bBottomCollapsed = bInCollapsed;
	BuildDocks();
}

bool EditorScene::AreTilesInsideDock() const
{
	if ( PaletteTiles.IsEmpty() )
	{
		return false;
	}
	for ( const LayoutWidget* const Tile : PaletteTiles )
	{
		const model::RectF Box = GetLocalBox(Tile);
		if ( !Layout.BottomDock.Contains(Box.Origin) || !Layout.BottomDock.Contains(Box.Origin + Box.Size) )
		{
			return false;
		}
	}
	return true;
}

bool EditorScene::IsEnemyStepperInsideDock() const
{
	if ( !EnemyStepper )
	{
		return false;
	}
	const model::RectF Box = GetLocalBox(EnemyStepper);
	return Layout.LeftDock.Contains(Box.Origin) && Layout.LeftDock.Contains(Box.Origin + Box.Size);
}

bool EditorScene::AreDockHandlesReachable() const
{
	for ( int Dock = 0; Dock < static_cast<int>(DockHandles.size()); ++Dock )
	{
		const LayoutWidget* const Handle = DockHandles[Dock];
		if ( !Handle || !Handle->isVisible() )
		{
			return false;
		}
		const model::RectF Box = GetLocalBox(Handle);
		if ( !GetDockRect(Dock).Contains(Box.Origin) || !GetDockRect(Dock).Contains(Box.Origin + Box.Size) )
		{
			return false;
		}
	}
	return true;
}

std::size_t EditorScene::CountLibraryLevelsThatFailToOpen(const model::LibrarySource InSource)
{
	std::size_t Failed = 0;
	for ( const model::LibraryEntry& Entry : model::FilterLibraryEntries(GetLibraryEntries(), InSource) )
	{
		LevelData Opened;
		if ( !LoadEntry(Entry, Opened) )
		{
			++Failed;
		}
	}
	return Failed;
}

void EditorScene::ToggleDock(const int InDock)
{
	bool& bCollapsed = InDock == LeftDock ? Docks.bLeftCollapsed : InDock == RightDock ? Docks.bRightCollapsed : Docks.bBottomCollapsed;
	const bool bShownCollapsed = InDock == LeftDock ? Layout.bLeftCollapsed : InDock == RightDock ? Layout.bRightCollapsed : Layout.bBottomCollapsed;
	bCollapsed = !bShownCollapsed;
	if ( bShownCollapsed )
	{
		Docks.bAutoCollapse = false;
	}
	RebuildLater();
}

void EditorScene::RebuildLater()
{
	runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(0.0f), cocos2d::CallFunc::create([this]()
	{
		BuildDocks();
		ApplyLayout();
	}), nullptr));
}

LayoutWidget* EditorScene::CreateDock(const int InDock, const model::RectF& InRect, const bool bInCollapsed, const TArray<std::string>& InTabKeys,
	const std::size_t InActiveTab, const TFunction<void(std::size_t), UiCallbackInlineSize>& InOnTab)
{
	const auto OnTab = MakeSharedValue<const TFunction<void(std::size_t), UiCallbackInlineSize>>(InOnTab);
	LayoutWidget* const Dock = LayoutWidget::Create(bInCollapsed ? DockStripId : ids::widgets::Dock);
	if ( !Dock )
	{
		return nullptr;
	}
	Dock->SetFixedSize(ToCocosSize(InRect.Size));
	Dock->setPosition(ToCocos(InRect.GetCenter()));
	DocksLayer->addChild(Dock);

	LayoutWidget* const Header = bInCollapsed ? Dock : Dock->FindWidget("header");
	LayoutWidget* const TabsRow = bInCollapsed || !Header ? nullptr : Header->FindWidget("tabs");
	LayoutWidget* const Handle = Header ? Header->FindWidget("handle") : nullptr;
	if ( TabsRow )
	{
		const TArray<LayoutWidget*> Tabs = Header->SetListCount("tabs", InTabKeys.Size());
		for ( std::size_t Index = 0; Index < Tabs.Size(); ++Index )
		{
			LayoutWidget* const TabWidget = Tabs[Index];
			TabWidget->SetText("text", Text(InTabKeys[Index]));
			const bool bActive = Index == InActiveTab;
			TabWidget->SetPartColors("background", bActive ? "button_pressed" : "panel", bActive ? "selection" : "button_border");
			if ( bActive )
			{
				TabWidget->SetTextColor("text", widgets::colors::TextNormal());
			}
			DockTabs[InDock].PushBack(TabWidget);
			TabWidget->SetClickHandler([OnTab, Index]() { OnTab->Get()(Index); });
		}
	}
	if ( Handle )
	{
		const char* const Open = InDock == LeftDock ? "<" : InDock == RightDock ? ">" : "v";
		const char* const Closed = InDock == LeftDock ? ">" : InDock == RightDock ? "<" : "^";
		Handle->SetText("text", bInCollapsed ? Closed : Open);
		if ( bInCollapsed )
		{
			Handle->SetFixedSize(cocos2d::Size(std::max(1.0f, InRect.Size.X - 8.0f), std::max(1.0f, InRect.Size.Y - 8.0f)));
			Dock->SetClickHandler([this, InDock]() { ToggleDock(InDock); });
		}
		Handle->SetClickHandler([this, InDock]() { ToggleDock(InDock); });
		DockHandles[InDock] = Handle;
	}
	return Dock;
}

void EditorScene::BuildDocks()
{
	DocksLayer->removeAllChildren();
	PaletteTiles.Clear();
	DockHandles.fill(nullptr);
	for ( auto& Tabs : DockTabs )
	{
		Tabs.Clear();
	}
	HistoryRows.Clear();
	LibraryRows.Clear();
	CampaignAddRows.Clear();
	EnemyStepper = nullptr;
	Layout = model::EditorLayout(Style().Layout).Compute(GetScreenBounds(), Docks);

	LayoutWidget* const Left = CreateDock(LeftDock, Layout.LeftDock, Layout.bLeftCollapsed, { "EDITOR_TAB_LIBRARY" }, LeftTab,
		[this](const std::size_t InTab) { LeftTab = InTab; RebuildLater(); });
	if ( Left && !Layout.bLeftCollapsed )
	{
		FillLeftBody(Left);
	}

	TArray<std::string> PaletteKeys;
	for ( const model::EditorTab& Tab : Palette.GetTabs() )
	{
		PaletteKeys.PushBack(Tab.NameKey);
	}
	LayoutWidget* const Bottom = CreateDock(BottomDock, Layout.BottomDock, Layout.bBottomCollapsed, PaletteKeys, ActiveTab,
		[this](const std::size_t InTab) { SelectTab(InTab); });
	if ( Bottom && !Layout.bBottomCollapsed )
	{
		FillBottomBody(Bottom);
	}

	LayoutWidget* const Right = CreateDock(RightDock, Layout.RightDock, Layout.bRightCollapsed, { "EDITOR_TAB_HISTORY", "EDITOR_TAB_SCRIPT" }, RightTab,
		[this](const std::size_t InTab) { RightTab = InTab; RebuildLater(); });
	if ( Right && !Layout.bRightCollapsed )
	{
		FillRightBody(Right);
	}
}

void EditorScene::FillLeftBody(LayoutWidget* const InDock)
{
	LayoutWidget* const Body = InDock->FindWidget("body");
	if ( !Body )
	{
		return;
	}
	const float Width = InDock->GetInnerWidth();
	FillLibraryBody(Body, Width);
}

void EditorScene::FillBottomBody(LayoutWidget* const InDock)
{
	LayoutWidget* const Body = InDock->FindWidget("body");
	LayoutWidget* const Panel = Body ? LayoutWidget::Create(ids::widgets::EditorPalette) : nullptr;
	if ( !Panel )
	{
		return;
	}
	Panel->SetFixedSize(cocos2d::Size(InDock->GetInnerWidth(), 0.0f));
	PaletteTiles = Panel->SetListCount("tiles", TabEntries.Size());
	for ( std::size_t Index = 0; Index < PaletteTiles.Size(); ++Index )
	{
		LayoutWidget* const TileWidget = PaletteTiles[Index];
		const model::EditorPaletteEntry& Entry = Palette.GetEntries()[TabEntries[Index]];
		if ( Entry.Icon == model::PaletteIcon::Erase )
		{
			TileWidget->SetPartColors("swatch", "bad", "");
		}
		else
		{
			TileWidget->SetIconFrame("icon", GetEntryFrame(Entry));
		}
		TileWidget->SetPartOpacity("selection", TabEntries[Index] == SelectedEntry ? 1.0f : 0.0f);
		TileWidget->SetClickHandler([this, Index]() { SelectItem(Index); });
	}
	Body->AppendNode(Panel);
}

void EditorScene::FillRightBody(LayoutWidget* const InDock)
{
	LayoutWidget* const Body = InDock->FindWidget("body");
	LayoutWidget* const Panel = Body ? LayoutWidget::Create(RightTab == 1 ? ids::widgets::EditorScript : ids::widgets::EditorHistory) : nullptr;
	if ( !Panel )
	{
		return;
	}
	Panel->SetFixedSize(cocos2d::Size(InDock->GetInnerWidth(), 0.0f));
	if ( RightTab == 1 )
	{
		Panel->SetText("info", Level.Script.Steps.IsEmpty() ? Text("EDITOR_SCRIPT_EMPTY") : std::to_string(Level.Script.Steps.Size()));
	}
	else
	{
		FillHistory(Panel);
	}
	Body->AppendNode(Panel);
}

void EditorScene::FillHistory(LayoutWidget* const InPanel)
{
	LayoutWidget* const Buttons = InPanel->FindWidget("buttons");
	LayoutWidget* const UndoButton = Buttons->FindWidget("first");
	UndoButton->SetEnabled(History.CanUndo());
	UndoButton->SetClickHandler([this]() { Undo(); });
	LayoutWidget* const RedoButton = Buttons->FindWidget("second");
	RedoButton->SetEnabled(History.CanRedo());
	RedoButton->SetClickHandler([this]() { Redo(); });

	const std::size_t Size = History.GetSize();
	const std::size_t Visible = std::min<std::size_t>(Size + 1, MaxHistoryRows);
	std::size_t First = History.GetStep() + 1 > Visible ? History.GetStep() + 1 - Visible : 0;
	First = std::min(First, Size + 1 - Visible);
	HistoryRows = InPanel->SetListCount("rows", Visible);
	for ( std::size_t Index = 0; Index < HistoryRows.Size(); ++Index )
	{
		const std::size_t Step = First + Index;
		const bool bStart = Step == 0;
		const model::EditorCommand* const Command = bStart ? nullptr : &History.GetCommand(Step - 1);
		LayoutWidget* const Line = HistoryRows[Index];
		SetRow(Line, bStart ? Text("EDITOR_HISTORY_START") : Text(std::string(model::GetHistoryTextKey(Command->GetKind()))),
			bStart ? std::string() : Command->GetDetail(), Step == History.GetStep(), [this, Step]() { JumpToStep(Step); });
		Line->SetPartOpacity("text", Step <= History.GetStep() ? 1.0f : FutureRowOpacity);
	}
}

void EditorScene::SelectTab(const std::size_t InTab)
{
	if ( InTab >= Palette.GetTabs().Size() )
	{
		return;
	}
	ActiveTab = InTab;
	TabEntries = Palette.GetEntriesOfTab(ActiveTab);
	SelectedEntry = TabEntries.IsEmpty() ? SelectedEntry : TabEntries.Front();
	if ( DocksLayer )
	{
		RebuildLater();
	}
}

void EditorScene::SelectItem(const std::size_t InTabItem)
{
	if ( InTabItem >= TabEntries.Size() )
	{
		return;
	}
	SelectedEntry = TabEntries[InTabItem];
	for ( std::size_t Index = 0; Index < PaletteTiles.Size() && Index < TabEntries.Size(); ++Index )
	{
		PaletteTiles[Index]->SetPartOpacity("selection", Index == InTabItem ? 1.0f : 0.0f);
	}
}

bool EditorScene::OnTouchBegan(const model::Vec2f& InPoint)
{
	Drag = DragMode::None;
	bFirstPoint = true;
	if ( MenuLayer && MenuLayer->getChildrenCount() > 0 )
	{
		CloseMenu();
		return true;
	}
	if ( Layout.HorizontalBar.Contains(InPoint) && Viewport.CanScrollX() )
	{
		Drag = DragMode::HorizontalBar;
	}
	else if ( Layout.VerticalBar.Contains(InPoint) && Viewport.CanScrollY() )
	{
		Drag = DragMode::VerticalBar;
	}
	else if ( Layout.Minimap.Contains(InPoint) && (Viewport.CanScrollX() || Viewport.CanScrollY()) )
	{
		Drag = DragMode::Minimap;
	}
	else if ( Layout.MapView.Contains(InPoint) )
	{
		Drag = DragMode::Paint;
	}
	if ( Drag == DragMode::None )
	{
		return false;
	}
	OnTouchMoved(InPoint);
	return true;
}

void EditorScene::OnTouchMoved(const model::Vec2f& InPoint)
{
	switch ( Drag )
	{
	case DragMode::Paint:
		if ( bFirstPoint || Tools.RepeatsOnDrag() )
		{
			ClickAt(InPoint);
		}
		bFirstPoint = false;
		break;
	case DragMode::HorizontalBar:
		Viewport.ScrollToHorizontalThumb(Layout.HorizontalBar, InPoint.X);
		ApplyScroll();
		break;
	case DragMode::VerticalBar:
		Viewport.ScrollToVerticalThumb(Layout.VerticalBar, InPoint.Y);
		ApplyScroll();
		break;
	case DragMode::Minimap:
		Viewport.CenterOn(Viewport.MinimapToContent(Layout.Minimap, InPoint));
		ApplyScroll();
		break;
	case DragMode::None:
		break;
	}
}

void EditorScene::OnTouchEnded()
{
	Drag = DragMode::None;
}

bool EditorScene::FindCellAt(const model::Vec2f& InPoint, model::EditorCell& OutCell) const
{
	if ( !Layout.MapView.Contains(InPoint) )
	{
		return false;
	}
	return Viewport.CellAt(Viewport.ViewToContent(InPoint - Layout.MapView.Origin), GetTileSize(), OutCell);
}

void EditorScene::ClickAt(const model::Vec2f& InPoint)
{
	model::EditorCell Cell;
	if ( !FindCellAt(InPoint, Cell) )
	{
		return;
	}
	if ( ActiveLayer == MapLayer::Points )
	{
		ClickPoint(Cell);
		return;
	}
	if ( ActiveLayer == MapLayer::Objects )
	{
		ClickObject(Cell);
		return;
	}
	if ( ActiveLayer == MapLayer::Floor )
	{
		ClickFloor(Cell);
		return;
	}
	if ( auto Command = Tools.Click(Document, Cell, GetSelectedTile()) )
	{
		Execute(std::move(Command));
	}
	UpdateSelectionMark();
	if ( Tools.HasSelection() && Tools.GetTool() == model::EditorTool::Select )
	{
		JumpToPaletteTile(Document.GetTile(Tools.GetSelection()));
	}
}

void EditorScene::UpdateSelectionMark()
{
	SelectionMark->clear();
	if ( !Tools.HasSelection() )
	{
		return;
	}
	const model::Vec2f Centre = Viewport.CellCenter(Tools.GetSelection(), GetTileSize());
	const float Half = GetTileSize() / 2;
	const model::Color Color = Tools.IsMovePending() ? model::Color(255, 140, 40) : model::Color(255, 205, 70);
	SelectionMark->drawRect(cocos2d::Vec2(Centre.X - Half, Centre.Y - Half), cocos2d::Vec2(Centre.X + Half, Centre.Y + Half), Paint(Color));
	SelectionMark->drawRect(cocos2d::Vec2(Centre.X - Half + 1, Centre.Y - Half + 1), cocos2d::Vec2(Centre.X + Half - 1, Centre.Y + Half - 1), Paint(Color));
}

void EditorScene::JumpToPaletteTile(const char InTile)
{
	const auto& Entries = Palette.GetEntries();
	for ( std::size_t Index = 0; Index < Entries.Size(); ++Index )
	{
		if ( Entries[Index].Tile == InTile )
		{
			SelectTab(Entries[Index].Tab);
			SelectedEntry = Index;
			return;
		}
	}
}

void EditorScene::SetTool(const model::EditorTool InTool)
{
	Tools.SetTool(InTool);
	UpdateSelectionMark();
}

void EditorScene::Undo()
{
	if ( History.Undo(Document) )
	{
		AfterHistoryChange();
	}
}

void EditorScene::Redo()
{
	if ( History.Redo(Document) )
	{
		AfterHistoryChange();
	}
}

void EditorScene::JumpToStep(const std::size_t InStep)
{
	if ( History.JumpTo(Document, InStep) )
	{
		AfterHistoryChange();
	}
}

void EditorScene::AfterHistoryChange()
{
	if ( ShownRows.Size() != Document.GetRows().Size() || (!ShownRows.IsEmpty() && ShownRows.Front().size() != Document.GetRows().Front().size()) )
	{
		ApplyLayout();
		RedrawAllCells();
	}
	else
	{
		SyncCells();
	}
	UpdateEnemyLabel();
	UpdateSelectionMark();
	RebuildLater();
}

void EditorScene::OpenMenu(const model::Vec2f& InPoint)
{
	CloseMenu();
	MenuLines.Clear();
	model::EditorCell Cell;
	const bool bOnCell = FindCellAt(InPoint, Cell);
	model::EditorMenuContext Context;
	Context.bOnObject = bOnCell && Document.GetTile(Cell) != tiles::Empty;
	Context.Tool = Tools.GetTool();
	Context.bCanUndo = History.CanUndo();
	Context.bCanRedo = History.CanRedo();

	LayoutWidget* const Menu = LayoutWidget::Create(ids::widgets::ContextMenu);
	if ( !Menu )
	{
		return;
	}
	TArray<LayoutWidget*> Entries;
	for ( const model::EditorMenuItem& Item : model::BuildEditorMenu(Context) )
	{
		if ( Item.bSeparatorBefore )
		{
			Menu->AppendWidget(LayoutWidget::Create(ids::widgets::MenuSeparator), true);
		}
		LayoutWidget* const Line = LayoutWidget::Create(ids::widgets::MenuItem);
		Line->SetText("text", Text(std::string(model::GetEditorMenuTextKey(Item.Action))));
		Line->SetPartVisible("key", false);
		if ( Item.bChecked )
		{
			Line->SetTextColor("text", widgets::colors::Accent());
			Line->SetPartOpacity("background", 0.6f);
		}
		if ( !Item.bEnabled )
		{
			Line->SetPartOpacity("text", 0.4f);
		}
		else
		{
			const model::EditorMenuAction Action = Item.Action;
			Line->SetClickHandler([this, Action, Cell]()
			{
				CloseMenu();
				RunMenuAction(Action, Cell);
			});
		}
		Entries.PushBack(Line);
		MenuLines.EmplaceBack(Item.Action, Line);
		Menu->AppendWidget(Line, true);
	}

	const cocos2d::Size Size = Menu->getContentSize();
	const model::RectF Screen = GetScreenBounds();
	const float Left = std::clamp(InPoint.X, Screen.GetMinX(), std::max(Screen.GetMinX(), Screen.GetMaxX() - Size.width));
	const float Top = std::clamp(InPoint.Y, Screen.GetMinY() + Size.height, Screen.GetMaxY());
	Menu->setPosition(Left + Size.width / 2, Top - Size.height / 2);
	MenuLayer->addChild(Menu);
	GameContext::Get<HoverManager>().SetModalLayer(Entries.IsEmpty() ? 0 : Entries.Front()->GetHoverLayer());
}

void EditorScene::CloseMenu()
{
	MenuLines.Clear();
	GameContext::Get<HoverManager>().SetModalLayer(0);
	if ( !MenuLayer )
	{
		return;
	}
	for ( cocos2d::Node* const Child : MenuLayer->getChildren() )
	{
		Child->setVisible(false);
		Child->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(0.0f), cocos2d::RemoveSelf::create(), nullptr));
	}
}

void EditorScene::RunMenuAction(const model::EditorMenuAction InAction, const model::EditorCell& InCell)
{
	model::EditorTool Tool;
	if ( model::GetToolOfAction(InAction, Tool) )
	{
		SetTool(Tool);
		return;
	}
	switch ( InAction )
	{
	case model::EditorMenuAction::GoToCategory:
		JumpToPaletteTile(Document.GetTile(InCell));
		break;
	case model::EditorMenuAction::Remove:
		Execute(MakeUnique<model::RemoveTileCommand>(InCell));
		break;
	case model::EditorMenuAction::MoveObject:
		Tools.BeginMove(Document, InCell);
		UpdateSelectionMark();
		break;
	case model::EditorMenuAction::Undo:
		Undo();
		break;
	case model::EditorMenuAction::Redo:
		Redo();
		break;
	default:
		break;
	}
}

void EditorScene::Execute(TUniquePtr<model::EditorCommand> InCommand)
{
	History.Execute(Document, std::move(InCommand));
	AfterHistoryChange();
}

void EditorScene::ResetMap()
{
	const int Total = Level.TotalEnemies;
	Level = LevelLoader::CreateEmpty();
	Level.TotalEnemies = Total;
	Level.MaxAlive = std::min(Total, Style().MaxAlive);
	Level.EnemyIds.Clear();
	for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
	{
		Level.EnemyIds.PushBack(Enemy.Id);
	}
	Level.Rows[Level.Rows.Size() - 1][Level.Rows.Front().size() / 2] = tiles::PlayerSpawn;
	Level.Rows[0][0] = tiles::EnemySpawn;

	Document = model::EditorDocument(Level.Rows, CollectUniqueTiles());
	Document.SetProperty(std::string(ShowEnemiesKey), std::to_string(Level.TotalEnemies));
	History.Clear();
	ShownRows = Document.GetRows();
	ApplyLayout();
}

void EditorScene::ChangeEnemyCount(const int InDelta)
{
	const int Count = std::atoi(Document.GetProperty(std::string(ShowEnemiesKey)).c_str());
	const int Next = std::max(Style().MinEnemies, std::min(Style().MaxEnemies, Count + InDelta));
	if ( Next != Count )
	{
		Execute(MakeUnique<model::SetPropertyCommand>(std::string(ShowEnemiesKey), std::to_string(Next)));
	}
}

void EditorScene::UpdateEnemyLabel()
{
	if ( EnemyStepper )
	{
		EnemyStepper->SetText("value", Document.GetProperty(std::string(ShowEnemiesKey)));
	}
}

void EditorScene::SyncLevelFromDocument()
{
	Level.Rows = Document.GetRows();
	Level.TotalEnemies = std::atoi(Document.GetProperty(std::string(ShowEnemiesKey)).c_str());
	Level.MaxAlive = std::min(Level.TotalEnemies, Style().MaxAlive);
}

bool EditorScene::CheckLevel()
{
	SyncLevelFromDocument();
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
	const bool bOriginal = bHasCurrent && Current.Source != model::LibrarySource::Custom;
	if ( model::ChooseSaveTarget(CanWriteSource(), bOriginal) == model::SaveTarget::SourceFile )
	{
		SaveToSourceFile();
	}
	else
	{
		SaveCustomCopy();
	}
}

std::string EditorScene::CollectUniqueTiles() const
{
	std::string UniqueTiles;
	for ( const model::LegendEntry& Entry : GameContext::Get<LevelLegendManager>().Get().GetEntries() )
	{
		if ( Entry.bUnique || Entry.Role == model::TileRole::Player )
		{
			UniqueTiles.push_back(Entry.Tile);
		}
	}
	return UniqueTiles;
}

void EditorScene::ApplyLevel(LevelData InLevel)
{
	Level = std::move(InLevel);
	if ( !LevelLoader::HasOwnPools(Level) )
	{
		Level.EnemyIds.Clear();
		for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
		{
			Level.EnemyIds.PushBack(Enemy.Id);
		}
	}
	Document = model::EditorDocument(Level.Rows, CollectUniqueTiles());
	Document.SetProperty(std::string(ShowEnemiesKey), std::to_string(Level.TotalEnemies));
	History.Clear();
	Tools.ClearSelection();
	ShownRows = Document.GetRows();
	ApplyLayout();
}

void EditorScene::NewLevel()
{
	StashCurrentDraft();
	bHasCurrent = false;
	NewKey = "new:" + std::to_string(++NewCounter);
	LevelData Empty = LevelLoader::CreateEmpty();
	Empty.Rows[Empty.Rows.Size() - 1][Empty.Rows.Front().size() / 2] = tiles::PlayerSpawn;
	Empty.Rows[0][0] = tiles::EnemySpawn;
	bHasCurrent = false;
	ApplyLevel(std::move(Empty));
}

TArray<model::LibraryEntry> EditorScene::GetLibraryEntries() const
{
	const auto ToOriginals = [](const std::string_view InGroup)
	{
		TArray<model::LibraryOriginal> Originals;
		for ( const auto& Entry : config_refs::ListGroupEntries(InGroup) )
		{
			Originals.PushBack(model::LibraryOriginal{ Entry.Id, Entry.Path });
		}
		return Originals;
	};
	return model::BuildLibraryEntries(ToOriginals(ids::config_groups::Levels), ToOriginals("cfg.test_level."), Library);
}

bool EditorScene::LoadEntry(const model::LibraryEntry& InEntry, LevelData& OutLevel) const
{
	if ( InEntry.Source != model::LibrarySource::Custom )
	{
		return LevelLoader::Load(InEntry.Path, OutLevel);
	}
	const std::optional<std::string> Text = GameContext::Get().GetStorage().ReadText(model::CustomLibrary::LevelFileName(InEntry.Id));
	const std::string ConfigId = "custom." + InEntry.Id;
	return Text && GameContext::Get<ConfigReader>().LoadConfigText(*Text, ConfigId) && LevelLoader::Load(ConfigId, OutLevel);
}

std::string EditorScene::GetCurrentKey() const
{
	return bHasCurrent ? std::to_string(static_cast<int>(Current.Source)) + ":" + Current.Id : NewKey;
}

void EditorScene::StashCurrentDraft()
{
	if ( History.GetSize() == 0 )
	{
		return;
	}
	SyncLevelFromDocument();
	Draft Saved{ Level, Document, std::move(History) };
	Drafts[GetCurrentKey()] = std::move(Saved);
	History = model::EditorHistory();
}

bool EditorScene::RestoreDraft(const std::string& InKey)
{
	auto* const FoundIt = Drafts.Find(InKey);
	if ( !FoundIt )
	{
		return false;
	}
	Level = std::move(FoundIt->Level);
	Document = std::move(FoundIt->Document);
	History = std::move(FoundIt->History);
	Drafts.Remove(FPersistentString(InKey));
	Tools.ClearSelection();
	ShownRows = Document.GetRows();
	ApplyLayout();
	return true;
}

std::size_t EditorScene::GetNewDraftCount() const
{
	return static_cast<std::size_t>(std::count_if(Drafts.begin(), Drafts.end(), [](const auto& InSaved) { return InSaved.Key.View().rfind("new:", 0) == 0; }));
}

void EditorScene::OpenNewDraft(const std::string& InKey)
{
	if ( !bHasCurrent && NewKey == InKey )
	{
		return;
	}
	StashCurrentDraft();
	bHasCurrent = false;
	NewKey = InKey;
	RestoreDraft(InKey);
}

bool EditorScene::HasDraft(const model::LibraryEntry& InEntry) const
{
	return Drafts.Find(std::to_string(static_cast<int>(InEntry.Source)) + ":" + InEntry.Id) != nullptr;
}

void EditorScene::OpenEntry(const model::LibraryEntry& InEntry)
{
	const std::string Key = std::to_string(static_cast<int>(InEntry.Source)) + ":" + InEntry.Id;
	if ( bHasCurrent && Current.Source == InEntry.Source && Current.Id == InEntry.Id )
	{
		return;
	}
	StashCurrentDraft();
	if ( Drafts.Find(Key) != nullptr )
	{
		Current = InEntry;
		bHasCurrent = true;
		RestoreDraft(Key);
		return;
	}
	LevelData Opened;
	if ( !LoadEntry(InEntry, Opened) )
	{
		ShowMessage(Text("EDITOR_LIB_OPEN_FAILED"), false);
		return;
	}
	Current = InEntry;
	bHasCurrent = true;
	ApplyLevel(std::move(Opened));
}

bool EditorScene::CanWriteSource() const
{
	return bHasDebugTools && !Style().DevResourcesRoot.empty() && std::filesystem::is_directory(Style().DevResourcesRoot);
}

void EditorScene::SaveToSourceFile()
{
	const std::string FilePath = model::JoinSourcePath(Style().DevResourcesRoot, Current.Path);
	std::ofstream File(FilePath, std::ios::binary | std::ios::trunc);
	File << LevelLoader::ToJson(Level);
	ShowMessage(File.good() ? Text("EDITOR_LIB_SAVED_SOURCE") : Text("EDITOR_SAVE_FAILED"), File.good());
}

void EditorScene::SaveCustomCopy()
{
	const bool bKeepId = bHasCurrent && Current.Source == model::LibrarySource::Custom;
	const std::string Id = bKeepId ? Current.Id : Library.MakeLevelId();
	ports::IStorage& Storage = GameContext::Get().GetStorage();
	if ( !Storage.WriteText(model::CustomLibrary::LevelFileName(Id), LevelLoader::ToJson(Level)) )
	{
		ShowMessage(Text("EDITOR_SAVE_FAILED"), false);
		return;
	}
	if ( !Library.HasLevel(Id) )
	{
		Library.AddLevel(Id, IsGroupEditable() ? LibraryGroup : std::string(model::CustomLibrary::DefaultCampaignId));
		Library.Save(Storage);
	}
	LevelLoader::SaveCustom(Level);
	Current = model::LibraryEntry{ model::LibrarySource::Custom, Id, Id, std::string() };
	bHasCurrent = true;
	ShowMessage(Text("EDITOR_SAVED"), true);
}

bool EditorScene::IsGroupEditable() const
{
	return LibraryGroup != GroupMain && LibraryGroup != GroupTest;
}

model::LibrarySource EditorScene::GetLibrarySource() const
{
	return LibraryGroup == GroupMain ? model::LibrarySource::Campaign : LibraryGroup == GroupTest ? model::LibrarySource::Test : model::LibrarySource::Custom;
}

void EditorScene::ChangeCampaign(const TFunction<void(model::CustomLibrary&), UiCallbackInlineSize>& InChange)
{
	InChange(Library);
	Library.Save(GameContext::Get().GetStorage());
	RebuildLater();
}

void EditorScene::FillLibraryBody(LayoutWidget* const InBody, const float InWidth)
{
	LayoutWidget* const Panel = LayoutWidget::Create(ids::widgets::EditorLibrary);
	if ( !Panel )
	{
		return;
	}
	Panel->SetFixedSize(cocos2d::Size(InWidth, 0.0f));
	if ( IsGroupEditable() && !Library.FindCampaign(LibraryGroup) )
	{
		LibraryGroup = std::string(model::CustomLibrary::DefaultCampaignId);
	}
	const auto SelectGroup = [this](const std::string& InGroup)
	{
		LibraryGroup = InGroup;
		LibraryPage = 0;
		CampaignLevel = 0;
		RebuildLater();
	};

	LayoutWidget* const Sources = Panel->FindWidget("sources");
	const std::array<std::pair<std::string_view, std::string>, 3> Groups{ { { "first", std::string(GroupMain) },
		{ "second", std::string(model::CustomLibrary::DefaultCampaignId) }, { "third", std::string(GroupTest) } } };
	for ( const auto& [ButtonId, Group] : Groups )
	{
		LayoutWidget* const SourceButton = Sources->FindWidget(ButtonId);
		SourceButton->SetEnabled(LibraryGroup != Group);
		SourceButton->SetClickHandler([SelectGroup, Group]() { SelectGroup(Group); });
	}

	TArray<const model::CampaignDef*> Campaigns;
	for ( const model::CampaignDef& Campaign : Library.GetCampaigns() )
	{
		if ( Campaign.Id != model::CustomLibrary::DefaultCampaignId )
		{
			Campaigns.PushBack(&Campaign);
		}
	}
	const TArray<LayoutWidget*> CampaignRows = Panel->SetListCount("campaigns", Campaigns.Size());
	for ( std::size_t Index = 0; Index < CampaignRows.Size(); ++Index )
	{
		const std::string Id = Campaigns[Index]->Id;
		SetRow(CampaignRows[Index], Campaigns[Index]->Name, std::to_string(Campaigns[Index]->LevelIds.Size()), Id == LibraryGroup, [SelectGroup, Id]() { SelectGroup(Id); });
	}
	Panel->BindClick("campaign_new", [this]()
	{
		ChangeCampaign([this](model::CustomLibrary& InLibrary)
		{
			LibraryGroup = InLibrary.AddCampaign(Text("EDITOR_CAMP_NAME") + " " + std::to_string(InLibrary.GetCampaigns().Size()));
			LibraryPage = 0;
			CampaignLevel = 0;
		});
	});

	const bool bEditable = IsGroupEditable();
	const bool bIsCustomHome = LibraryGroup == model::CustomLibrary::DefaultCampaignId;
	const TArray<model::LibraryEntry> Shown = bEditable ? model::BuildCampaignEntries(*Library.FindCampaign(LibraryGroup))
		: model::FilterLibraryEntries(GetLibraryEntries(), GetLibrarySource());
	const std::size_t Rows = static_cast<std::size_t>(std::max(1, Style().LibraryRows));
	const std::size_t Pages = std::max<std::size_t>(1, (Shown.Size() + Rows - 1) / Rows);
	LibraryPage = std::min(LibraryPage, Pages - 1);
	Panel->SetFlag("add_levels", false);
	Panel->SetFlag("empty", Shown.IsEmpty());
	Panel->SetFlag("paged", Pages > 1);
	Panel->SetFlag("campaign_levels", bEditable);
	Panel->SetFlag("campaign_custom", bEditable && !bIsCustomHome);

	TArray<std::string> Unsaved;
	if ( bIsCustomHome )
	{
		for ( const auto& Saved : Drafts )
		{
			if ( Saved.Key.View().rfind("new:", 0) == 0 )
			{
				Unsaved.PushBack(std::string(Saved.Key.View()));
			}
		}
		if ( !bHasCurrent && History.GetSize() > 0 )
		{
			Unsaved.PushBack(NewKey);
		}
	}
	const std::size_t FirstShown = LibraryPage * Rows;
	const std::size_t ShownCount = std::min(Shown.Size(), (LibraryPage + 1) * Rows) - FirstShown;
	LibraryRows = Panel->SetListCount("levels", ShownCount + Unsaved.Size());
	for ( std::size_t Row = 0; Row < ShownCount; ++Row )
	{
		const std::size_t Index = FirstShown + Row;
		const model::LibraryEntry Entry = Shown[Index];
		const bool bIsCurrent = bHasCurrent && Current.Source == Entry.Source && Current.Id == Entry.Id;
		const bool bIsMarked = bEditable && Index == CampaignLevel;
		const bool bIsEdited = HasDraft(Entry) || (bIsCurrent && History.GetSize() > 0);
		SetRow(LibraryRows[Row], (bEditable ? std::to_string(Index + 1) + ". " : std::string()) + Entry.Title, bIsMarked ? "*" : bIsEdited ? "~" : std::string(), bIsCurrent,
			[this, Entry, Index]() { CampaignLevel = Index; OpenEntry(Entry); RebuildLater(); });
	}
	for ( std::size_t Row = 0; Row < Unsaved.Size(); ++Row )
	{
		const std::string Key = Unsaved[Row];
		SetRow(LibraryRows[ShownCount + Row], Text("EDITOR_NEW_DRAFT") + " " + Key.substr(Key.find(':') + 1), "~", Key == NewKey && !bHasCurrent,
			[this, Key]() { OpenNewDraft(Key); RebuildLater(); });
	}
	LayoutWidget* const Pager = Panel->FindWidget("pager");
	Pager->FindWidget("first")->SetEnabled(LibraryPage > 0);
	Pager->BindClick("first", [this]() { --LibraryPage; RebuildLater(); });
	Pager->FindWidget("second")->SetEnabled(LibraryPage + 1 < Pages);
	Pager->BindClick("second", [this]() { ++LibraryPage; RebuildLater(); });

	CampaignAddRows.Clear();
	if ( bEditable )
	{
		FillCampaignTools(Panel);
	}
	Panel->BindClick("level_new", [this]() { NewLevel(); RebuildLater(); });
	FillLevelGroup(Panel->FindWidget("level"));
	InBody->AppendNode(Panel);
}

void EditorScene::FillCampaignTools(LayoutWidget* const InPanel)
{
	const std::string Id = LibraryGroup;
	const TArray<std::string> Levels = Library.FindCampaign(Id)->LevelIds;
	const bool bIsCustomHome = Id == model::CustomLibrary::DefaultCampaignId;
	const std::size_t Marked = std::min(CampaignLevel, Levels.IsEmpty() ? std::size_t(0) : Levels.Size() - 1);
	const auto Reorder = [this, Id, Levels, Marked](const int InStep)
	{
		TArray<std::string> Order = Levels;
		const std::size_t Target = static_cast<std::size_t>(static_cast<int>(Marked) + InStep);
		std::swap(Order[Marked], Order[Target]);
		CampaignLevel = Target;
		ChangeCampaign([Id, Order](model::CustomLibrary& InLibrary) { InLibrary.SetCampaignLevels(Id, Order); });
	};
	LayoutWidget* const Moves = InPanel->FindWidget("moves");
	Moves->SetPartVisible("third", !bIsCustomHome);
	Moves->FindWidget("first")->SetEnabled(!Levels.IsEmpty() && Marked > 0);
	Moves->BindClick("first", [Reorder]() { Reorder(-1); });
	Moves->FindWidget("second")->SetEnabled(!Levels.IsEmpty() && Marked + 1 < Levels.Size());
	Moves->BindClick("second", [Reorder]() { Reorder(1); });
	Moves->FindWidget("third")->SetEnabled(!Levels.IsEmpty());
	Moves->BindClick("third", [this, Levels, Marked]()
	{
		const std::string LevelId = Levels[Marked];
		ChangeCampaign([LevelId](model::CustomLibrary& InLibrary) { InLibrary.AssignLevel(LevelId, model::CustomLibrary::DefaultCampaignId); });
	});
	if ( bIsCustomHome )
	{
		return;
	}

	const model::CampaignDef* const Home = Library.FindCampaign(model::CustomLibrary::DefaultCampaignId);
	const TArray<std::string> HomeLevels = Home ? Home->LevelIds : TArray<std::string>();
	InPanel->SetFlag("add_levels", !HomeLevels.IsEmpty());
	const std::size_t Rows = static_cast<std::size_t>(std::max(1, Style().LibraryRows));
	CampaignAddRows = InPanel->SetListCount("add_rows", std::min(HomeLevels.Size(), Rows));
	for ( std::size_t Index = 0; Index < CampaignAddRows.Size(); ++Index )
	{
		const std::string LevelId = HomeLevels[Index];
		SetRow(CampaignAddRows[Index], LevelId, "+", false, [this, Id, LevelId]()
		{
			CampaignLevel = 0;
			ChangeCampaign([Id, LevelId](model::CustomLibrary& InLibrary) { InLibrary.AssignLevel(LevelId, Id); });
		});
	}
	InPanel->BindClick("campaign_delete", [this, Id]()
	{
		ChangeCampaign([this, Id](model::CustomLibrary& InLibrary)
		{
			const TArray<std::string> Orphans = InLibrary.FindCampaign(Id)->LevelIds;
			for ( const std::string& LevelId : Orphans )
			{
				InLibrary.AssignLevel(LevelId, model::CustomLibrary::DefaultCampaignId);
			}
			InLibrary.RemoveCampaign(Id);
			LibraryGroup = std::string(model::CustomLibrary::DefaultCampaignId);
		});
	});
}

void EditorScene::BindZoomSlider(LayoutWidget* const InRow)
{
	const float MinZoom = Style().MinZoom;
	const float Span = std::log(Style().MaxZoom / MinZoom);
	const auto ShowZoom = [this, InRow]()
	{
		InRow->SetText("value", (bAutoZoom ? Text("EDITOR_AUTO") + " " : std::string()) + std::to_string(static_cast<int>(std::lround(Zoom * 100.0f))) + "%");
	};
	ShowZoom();
	LayoutWidget* const Slider = InRow->FindWidget("slider");
	Slider->SetSliderValue(std::log(Zoom / MinZoom) / Span);
	Slider->SetSliderHandler([this, MinZoom, Span, ShowZoom](const float InValue)
	{
		SetZoom(MinZoom * std::exp(InValue * Span));
		ShowZoom();
	});
}

void EditorScene::FillLevelGroup(LayoutWidget* const InLevel)
{
	EnemyStepper = InLevel->FindWidget("enemies");
	BindZoomSlider(InLevel->FindWidget("zoom"));
	SetStepper(InLevel, "enemies", std::string(), [this]() { ChangeEnemyCount(-1); }, [this]() { ChangeEnemyCount(1); });
	UpdateEnemyLabel();

	const auto BindLayoutStepper = [this, InLevel](const std::string_view InId, const std::string& InValue, const int InKind)
	{
		SetStepper(InLevel, InId, InValue, [this, InKind]() { StepLayoutValue(InKind, -1); }, [this, InKind]() { StepLayoutValue(InKind, 1); });
	};
	BindLayoutStepper("width", std::to_string(Document.GetColumnCount()), 0);
	BindLayoutStepper("height", std::to_string(Document.GetRowCount()), 1);
	const char* const LayerKey = ActiveLayer == MapLayer::Points ? "EDITOR_LAYER_POINTS" : ActiveLayer == MapLayer::Objects ? "EDITOR_LAYER_OBJECTS" : ActiveLayer == MapLayer::Floor ? "EDITOR_LAYER_FLOOR" : "EDITOR_LAYER_GROUND";
	BindLayoutStepper("layer", Text(LayerKey), 2);
	InLevel->SetFlag("layer_points", ActiveLayer == MapLayer::Points);
	InLevel->SetFlag("layer_floor", ActiveLayer == MapLayer::Floor);
	BindLayoutStepper("point", SelectedPoint.empty() ? Text("EDITOR_POINT_NONE") : SelectedPoint, 3);
	const model::GroundEntry* const Floor = GameContext::Get<LevelLegendManager>().Get().FindGround(SelectedFloor);
	BindLayoutStepper("floor", Text(Floor ? Floor->NameKey : std::string("GROUND_NONE")), 4);
	SyncTileset();
	SetStepper(InLevel, "tileset", Tileset, [this]() { ChangeTileset(-1); }, [this]() { ChangeTileset(1); });

	InLevel->BindClick("fit", [this]() { FitMap(); RebuildLater(); });
	InLevel->BindClick("save", [this]() { Save(); });
	InLevel->BindClick("play", [this]() { Test(); });
	InLevel->BindClick("clear", [this]() { ResetMap(); });
	InLevel->BindClick("back", []() { SwitchScene(MenuScene::CreateScene()); });
}

void EditorScene::SyncTileset()
{
	const ResourceManifest Manifest;
	const std::string Wanted = Manifest.GetTilesetName(Level.Tileset);
	if ( Wanted != Tileset )
	{
		Tileset = Wanted;
		Atlases.AddAtlas(Manifest.GetTilesetAtlas(Tileset));
	}
}

void EditorScene::ChangeTileset(const int InStep)
{
	const ResourceManifest Manifest;
	const TArray<std::string> Names = Manifest.GetTilesetNames();
	if ( Names.IsEmpty() )
	{
		return;
	}
	int Index = 0;
	for ( int I = 0; I < static_cast<int>(Names.Size()); ++I )
	{
		if ( Names[I] == Tileset )
		{
			Index = I;
		}
	}
	const int Count = static_cast<int>(Names.Size());
	Level.Tileset = Names[(Index + InStep + Count) % Count];
	SyncTileset();
	RedrawAllCells();
	RebuildLater();
}

void EditorScene::RedrawAllCells()
{
	for ( auto Found = CellNodes.begin(); Found != CellNodes.end(); ++Found )
	{
		Found->Value->removeFromParent();
	}
	CellNodes.Clear();
	ShownRows = Document.GetRows();
	UpdateVisibleCells();
	RedrawMinimapTiles();
}

void EditorScene::ResizeMap(const int InColumnsDelta, const int InRowsDelta)
{
	const int Columns = Document.GetColumnCount() + InColumnsDelta;
	const int Rows = Document.GetRowCount() + InRowsDelta;
	if ( Columns < model::EditorDocument::MinSide || Rows < model::EditorDocument::MinSide || Columns > model::EditorDocument::MaxSide || Rows > model::EditorDocument::MaxSide )
	{
		return;
	}
	Execute(MakeUnique<model::ResizeMapCommand>(Rows, Columns, model::ResizeAnchor::TopLeft));
	Level.Layers.Crop(Columns, Rows, 0, 0);
	ResizeGround(Rows, Columns);
}

void EditorScene::StepLayoutValue(const int InKind, const int InStep)
{
	if ( InKind == 0 )
	{
		ResizeMap(InStep, 0);
	}
	else if ( InKind == 1 )
	{
		ResizeMap(0, InStep);
	}
	else if ( InKind == 2 )
	{
		StepLayer(InStep);
	}
	else if ( InKind == 3 )
	{
		StepPointName(InStep);
	}
	else
	{
		StepFloorTile(InStep);
	}
}

void EditorScene::StepLayer(const int InStep)
{
	constexpr int LayerCount = 4;
	const int Next = (static_cast<int>(ActiveLayer) + InStep + LayerCount) % LayerCount;
	ActiveLayer = static_cast<MapLayer>(Next);
	SelectedPoint.clear();
	RedrawMapChrome();
	RebuildLater();
}

void EditorScene::StepPointName(const int InStep)
{
	const TArray<std::string>& Names = Palette.GetPointNames();
	const model::LevelPoint* const Current = Level.Layers.FindPoint(SelectedPoint);
	if ( !Current || Names.IsEmpty() )
	{
		return;
	}
	std::size_t Index = 0;
	for ( std::size_t Candidate = 0; Candidate < Names.Size(); ++Candidate )
	{
		if ( Names[Candidate] == SelectedPoint )
		{
			Index = Candidate;
		}
	}
	const int Count = static_cast<int>(Names.Size());
	for ( int Try = 1; Try <= Count; ++Try )
	{
		const std::string& Name = Names[(static_cast<int>(Index) + InStep * Try % Count + Count) % Count];
		if ( Level.Layers.RenamePoint(SelectedPoint, Name) )
		{
			SelectedPoint = Name;
			break;
		}
	}
	RedrawMapChrome();
	RebuildLater();
}

void EditorScene::ClickPoint(const model::EditorCell& InCell)
{
	const model::LevelPoint* const Existing = Level.Layers.FindPointAt(InCell.Column, InCell.Row);
	if ( Tools.GetTool() == model::EditorTool::Eraser )
	{
		if ( Existing )
		{
			const std::string Name = Existing->Name;
			Level.Layers.RemovePoint(Name);
			SelectedPoint.clear();
			ShowMessage(Name, false);
		}
	}
	else if ( Existing )
	{
		SelectedPoint = Existing->Name;
	}
	else
	{
		SelectedPoint = Level.Layers.MakePointName();
		Level.Layers.SetPoint(SelectedPoint, InCell.Column, InCell.Row);
		ShowMessage(SelectedPoint, true);
	}
	RedrawMapChrome();
	RebuildLater();
}

void EditorScene::StepFloorTile(const int InStep)
{
	const TArray<model::GroundEntry>& Grounds = GameContext::Get<LevelLegendManager>().Get().GetGroundEntries();
	const int Count = static_cast<int>(Grounds.Size()) + 1;
	int Current = 0;
	for ( std::size_t Index = 0; Index < Grounds.Size(); ++Index )
	{
		if ( Grounds[Index].Tile == SelectedFloor )
		{
			Current = static_cast<int>(Index) + 1;
		}
	}
	const int Next = (Current + InStep + Count) % Count;
	SelectedFloor = Next == 0 ? '.' : Grounds[Next - 1].Tile;
	RebuildLater();
}

void EditorScene::ResizeGround(const int InRows, const int InColumns)
{
	if ( Level.Ground.IsEmpty() )
	{
		return;
	}
	TArray<std::string> Resized;
	for ( int Row = 0; Row < InRows; ++Row )
	{
		std::string Line = Row < static_cast<int>(Level.Ground.Size()) ? Level.Ground[Row] : std::string();
		Line.resize(static_cast<std::size_t>(InColumns), '.');
		Resized.PushBack(Line);
	}
	Level.Ground = std::move(Resized);
}

void EditorScene::ClickFloor(const model::EditorCell& InCell)
{
	const int Rows = Document.GetRowCount();
	const int Columns = Document.GetColumnCount();
	if ( Level.Ground.IsEmpty() )
	{
		for ( int Row = 0; Row < Rows; ++Row )
		{
			Level.Ground.PushBack(std::string(static_cast<std::size_t>(Columns), '.'));
		}
	}
	ResizeGround(Rows, Columns);
	Level.Ground[InCell.Row][InCell.Column] = Tools.GetTool() == model::EditorTool::Eraser ? '.' : SelectedFloor;
	const bool bAnyFloor = std::any_of(Level.Ground.begin(), Level.Ground.end(), [](const std::string& InLine) { return InLine.find_first_not_of('.') != std::string::npos; });
	if ( !bAnyFloor )
	{
		Level.Ground.Clear();
	}
	RedrawMapChrome();
}

void EditorScene::DrawFloorTint()
{
	const model::LevelLegend& Legend = GameContext::Get<LevelLegendManager>().Get();
	const float Tile = GetTileSize();
	const model::Vec2f Scroll = Viewport.GetScroll();
	for ( std::size_t Row = 0; Row < Level.Ground.Size(); ++Row )
	{
		for ( std::size_t Column = 0; Column < Level.Ground[Row].size(); ++Column )
		{
			const model::GroundEntry* const Floor = Legend.FindGround(Level.Ground[Row][Column]);
			if ( !Floor )
			{
				continue;
			}
			const model::Vec2f Centre = Viewport.CellCenter(model::EditorCell{ static_cast<int>(Row), static_cast<int>(Column) }, Tile);
			const model::Vec2f Screen = Layout.MapView.Origin + Centre - Scroll;
			if ( !Layout.MapView.Contains(Screen) )
			{
				continue;
			}
			const float Half = Tile * 0.5f;
			MapChrome->drawSolidRect(cocos2d::Vec2(Screen.X - Half, Screen.Y - Half), cocos2d::Vec2(Screen.X + Half, Screen.Y + Half), Paint(model::Color(Floor->Tint.R, Floor->Tint.G, Floor->Tint.B, 110)));
		}
	}
}

void EditorScene::ClickObject(const model::EditorCell& InCell)
{
	Level.Layers.RemoveObjectAt(InCell.Column, InCell.Row);
	if ( Tools.GetTool() != model::EditorTool::Eraser )
	{
		const model::LegendEntry* const Entry = GameContext::Get<LevelLegendManager>().Get().Find(GetSelectedTile());
		if ( Entry && !Entry->Config.empty() )
		{
			Level.Layers.AddObject(model::LevelObject{ Entry->Config, std::string(), InCell.Column, InCell.Row });
		}
	}
	RedrawMapChrome();
}

void EditorScene::DrawObjectMarkers()
{
	const float Tile = GetTileSize();
	const model::Vec2f Scroll = Viewport.GetScroll();
	const cocos2d::Color4F Color = Paint(model::Color(90, 210, 255));
	for ( const model::LevelObject& Object : Level.Layers.GetObjects() )
	{
		const model::Vec2f Centre = Viewport.CellCenter(model::EditorCell{ Object.Row, Object.Column }, Tile);
		const model::Vec2f Screen = Layout.MapView.Origin + Centre - Scroll;
		if ( !Layout.MapView.Contains(Screen) )
		{
			continue;
		}
		const float Half = Tile * 0.4f;
		MapChrome->drawLine(cocos2d::Vec2(Screen.X - Half, Screen.Y), cocos2d::Vec2(Screen.X, Screen.Y + Half), Color);
		MapChrome->drawLine(cocos2d::Vec2(Screen.X, Screen.Y + Half), cocos2d::Vec2(Screen.X + Half, Screen.Y), Color);
		MapChrome->drawLine(cocos2d::Vec2(Screen.X + Half, Screen.Y), cocos2d::Vec2(Screen.X, Screen.Y - Half), Color);
		MapChrome->drawLine(cocos2d::Vec2(Screen.X, Screen.Y - Half), cocos2d::Vec2(Screen.X - Half, Screen.Y), Color);
	}
}

void EditorScene::DrawPointMarkers()
{
	const float Tile = GetTileSize();
	const model::Vec2f Scroll = Viewport.GetScroll();
	for ( const model::LevelPoint& Point : Level.Layers.GetPoints() )
	{
		const model::Vec2f Centre = Viewport.CellCenter(model::EditorCell{ Point.Row, Point.Column }, Tile);
		const model::Vec2f Screen = Layout.MapView.Origin + Centre - Scroll;
		if ( !Layout.MapView.Contains(Screen) )
		{
			continue;
		}
		const float Half = Tile * 0.35f;
		const cocos2d::Color4F Color = Paint(Point.Name == SelectedPoint ? model::Color(255, 255, 255) : model::Color(255, 205, 70));
		MapChrome->drawRect(cocos2d::Vec2(Screen.X - Half, Screen.Y - Half), cocos2d::Vec2(Screen.X + Half, Screen.Y + Half), Color);
		MapChrome->drawLine(cocos2d::Vec2(Screen.X - Half, Screen.Y), cocos2d::Vec2(Screen.X + Half, Screen.Y), Color);
		MapChrome->drawLine(cocos2d::Vec2(Screen.X, Screen.Y - Half), cocos2d::Vec2(Screen.X, Screen.Y + Half), Color);
	}
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
	const viewmodel::WidgetStyle& Toast = GameContext::Get<UiStyle>().GetWidgets();
	auto CurrentLabel = widgets::CreateLabel(InText, Toast.ToastFontSize, bInGood ? widgets::colors::Good() : widgets::colors::Bad());
	CurrentLabel->setPosition(GetScreenBounds().GetCenter().X, Style().MessageBottom);
	widgets::AddOutline(CurrentLabel);
	addChild(CurrentLabel, ZMessage);
	CurrentLabel->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(Toast.ToastShowTime), cocos2d::FadeOut::create(Toast.ToastFadeTime),
		cocos2d::RemoveSelf::create(), nullptr));
}

} // namespace game::view::cocos

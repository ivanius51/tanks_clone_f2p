#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cocos2d.h"

#include "game/LevelLoader.h"
#include "model/EditorDocument.h"
#include "model/EditorHistory.h"
#include "model/EditorLayout.h"
#include "model/EditorPalette.h"
#include "model/MapViewport.h"
#include "scenes/TypedScene.h"
#include "services/ResourceHolder.h"
#include "ui/LayoutWidget.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

/**
 * The level editor: a left dock (Library, Level), a bottom dock (Palette) and a right dock (History, Script) built from
 * widget files, around a map that scrolls (wheel, middle-button drag, scrollbars, minimap) for any map size. Every dock
 * collapses to a strip with its handle. Map edits go through editor commands, so they can be undone.
 */
class EditorScene : public TypedScene<SceneKind::Editor>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(EditorScene);

	std::size_t GetActiveTab() const { return ActiveTab; }
	std::size_t GetVisibleItemCount() const { return TabEntries.size(); }
	const model::EditorPalette& GetPalette() const { return Palette; }
	char GetSelectedTile() const;
	const model::EditorDocument& GetDocument() const { return Document; }
	const model::EditorHistory& GetHistory() const { return History; }
	const model::MapViewport& GetViewport() const { return Viewport; }
	const model::EditorLayoutResult& GetLayout() const { return Layout; }

	void SelectTab(std::size_t InTab);
	void SelectItem(std::size_t InTabItem);
	void ToggleDock(int InDock);
	void ScrollMap(const model::Vec2f& InDelta);

	static constexpr int LeftDock = 0;
	static constexpr int RightDock = 1;
	static constexpr int BottomDock = 2;

private:
	void LoadPalette();
	void BuildMap();
	void BuildDocks();
	LayoutWidget* CreateDock(int InDock, const model::RectF& InRect, bool bInCollapsed, const std::vector<std::string>& InTabKeys, std::size_t InActiveTab,
		const std::function<void(std::size_t)>& InOnTab);
	void FillLeftBody(LayoutWidget* InDock);
	void FillBottomBody(LayoutWidget* InDock);
	void FillRightBody(LayoutWidget* InDock);
	void ApplyLayout();

	cocos2d::Node* CreateTileNode(char InTile) const;
	std::string GetEntryFrame(const model::EditorPaletteEntry& InEntry) const;

	void UpdateVisibleCells();
	void RefreshCell(const model::EditorCell& InCell);
	void SyncCells();
	void RedrawMapChrome();
	void RedrawMinimapTiles();
	void ApplyScroll();

	bool OnTouchBegan(const model::Vec2f& InPoint);
	void OnTouchMoved(const model::Vec2f& InPoint);
	void OnTouchEnded();
	model::Vec2f ToLayoutSpace(const cocos2d::Vec2& InWorldPoint) const;
	void PaintAt(const model::Vec2f& InPoint);

	void Execute(std::unique_ptr<model::EditorCommand> InCommand);
	void ChangeEnemyCount(int InDelta);
	void UpdateEnemyLabel();
	void ResetMap();
	bool CheckLevel();
	void SyncLevelFromDocument();
	void Save();
	void Test();
	void ShowMessage(const std::string& InText, bool bInGood);

	enum class DragMode
	{
		None,
		Paint,
		HorizontalBar,
		VerticalBar,
		Minimap
	};

	ResourceHolder Atlases;
	std::string Tileset;
	LevelData Level;
	model::EditorPalette Palette;
	model::EditorDocument Document;
	model::EditorHistory History;
	model::MapViewport Viewport;
	model::EditorLayoutResult Layout;
	model::EditorDockState Docks;
	std::vector<std::string> ShownRows;

	cocos2d::ClippingRectangleNode* MapClip = nullptr;
	cocos2d::Node* MapContent = nullptr;
	cocos2d::DrawNode* MapGround = nullptr;
	cocos2d::DrawNode* MapChrome = nullptr;
	cocos2d::DrawNode* MinimapTiles = nullptr;
	cocos2d::Node* DocksLayer = nullptr;
	std::unordered_map<long long, cocos2d::Node*> CellNodes;
	cocos2d::Label* EnemyLabel = nullptr;

	std::vector<LayoutWidget*> PaletteTiles;
	std::size_t LeftTab = 1;
	std::size_t RightTab = 0;
	std::vector<std::size_t> TabEntries; // palette entries shown on the active tab
	std::size_t ActiveTab = 0;
	std::size_t SelectedEntry = 0;
	DragMode Drag = DragMode::None;
	bool bPanning = false;
	model::Vec2f PanFrom;
};

} // namespace tanks::view::cocos

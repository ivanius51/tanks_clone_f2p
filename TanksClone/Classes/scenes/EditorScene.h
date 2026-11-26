#pragma once

#include <array>
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "cocos2d.h"

#include "game/LevelLoader.h"
#include "model/CustomLibrary.h"
#include "model/EditorDocument.h"
#include "model/EditorHistory.h"
#include "model/EditorLayout.h"
#include "model/EditorLibrary.h"
#include "model/EditorPalette.h"
#include "model/EditorTools.h"
#include "model/MapViewport.h"
#include "scenes/TypedScene.h"
#include "services/ResourceHolder.h"
#include "templates/TUniquePtr.h"
#include "ui/LayoutWidget.h"
#include "ui/Widgets.h"

namespace game::view::cocos
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
	std::size_t GetVisibleItemCount() const { return TabEntries.Size(); }
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
	model::EditorTool GetTool() const { return Tools.GetTool(); }
	void SetTool(model::EditorTool InTool);
	bool IsMenuOpen() const { return MenuLayer != nullptr && MenuLayer->getChildrenCount() > 0; }
	void Undo();
	void Redo();
	void JumpToStep(std::size_t InStep);

	void SetAllDocksCollapsed(bool bInCollapsed);
	void SetAutoCollapse(bool bInEnabled);
	bool AreTilesInsideDock() const;
	bool IsEnemyStepperInsideDock() const;
	bool AreDockHandlesReachable() const;
	std::size_t CountLibraryLevelsThatFailToOpen(model::LibrarySource InSource);

	cocos2d::Vec2 LayoutToWorld(const model::Vec2f& InPoint) const;
	cocos2d::Vec2 CellToWorld(const model::EditorCell& InCell) const;
	cocos2d::Vec2 GetWorldCentre(const cocos2d::Node* InNode) const;
	LayoutWidget* GetDockHandle(const int InDock) const { return DockHandles[InDock]; }
	LayoutWidget* GetDockTab(int InDock, std::size_t InIndex) const;
	LayoutWidget* GetMenuLine(model::EditorMenuAction InAction) const;
	LayoutWidget* GetHistoryRow(std::size_t InStep) const;
	LayoutWidget* GetCampaignAddRow(std::size_t InIndex) const { return InIndex < CampaignAddRows.Size() ? CampaignAddRows[InIndex] : nullptr; }
	const std::string& GetLibraryGroup() const { return LibraryGroup; }
	LayoutWidget* GetLibraryRow(std::size_t InIndex) const;
	bool IsDockCollapsed(int InDock) const;
	std::size_t GetRightTab() const { return RightTab; }
	bool HasOpenedLevel() const { return bHasCurrent; }
	const model::LibraryEntry& GetOpenedEntry() const { return Current; }
	const model::CustomLibrary& GetCustomLibrary() const { return Library; }
	int GetEnemyCount() const;
	float GetTileSize() const;
	float GetZoom() const { return Zoom; }
	void SetZoom(float InZoom);
	void FitMap();
	bool IsAutoZoom() const { return bAutoZoom; }
	std::size_t GetNewDraftCount() const;
	LayoutWidget* GetEnemyStepperButton(bool bInPlus) const;
	model::Vec2f GetMapScroll() const { return Viewport.GetScroll(); }

	static constexpr int LeftDock = 0;
	static constexpr int RightDock = 1;
	static constexpr int BottomDock = 2;

private:
	void LoadPalette();
	void BuildMap();
	void BuildDocks();
	LayoutWidget* CreateDock(int InDock, const model::RectF& InRect, bool bInCollapsed, const TArray<std::string>& InTabKeys, std::size_t InActiveTab,
		const TFunction<void(std::size_t), UiCallbackInlineSize>& InOnTab);
	void FillLeftBody(LayoutWidget* InDock);
	void FillBottomBody(LayoutWidget* InDock);
	void FillRightBody(LayoutWidget* InDock);
	void ApplyLayout();
	float ComputeFitZoom() const;
	const model::RectF& GetDockRect(int InDock) const;
	model::RectF GetLocalBox(const cocos2d::Node* InNode) const;

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
	void ClickAt(const model::Vec2f& InPoint);
	bool FindCellAt(const model::Vec2f& InPoint, model::EditorCell& OutCell) const;
	void UpdateSelectionMark();
	void JumpToPaletteTile(char InTile);
	void RebuildLater();
	void AfterHistoryChange();
	void OpenMenu(const model::Vec2f& InPoint);
	void CloseMenu();
	void RunMenuAction(model::EditorMenuAction InAction, const model::EditorCell& InCell);

	void Execute(TUniquePtr<model::EditorCommand> InCommand);
	void ChangeEnemyCount(int InDelta);
	void UpdateEnemyLabel();
	void ResetMap();
	bool CheckLevel();
	void SyncLevelFromDocument();
	void Save();
	void SaveToSourceFile();
	void SaveCustomCopy();
	std::string CollectUniqueTiles() const;
	void ApplyLevel(LevelData InLevel);
	void NewLevel();
	std::string GetCurrentKey() const;
	void StashCurrentDraft();
	bool RestoreDraft(const std::string& InKey);
	void OpenNewDraft(const std::string& InKey);
	bool HasDraft(const model::LibraryEntry& InEntry) const;
	TArray<model::LibraryEntry> GetLibraryEntries() const;
	void OpenEntry(const model::LibraryEntry& InEntry);
	bool LoadEntry(const model::LibraryEntry& InEntry, LevelData& OutLevel) const;
	bool CanWriteSource() const;
	void FillLibraryBody(LayoutWidget* InBody, float InWidth);
	void FillLevelGroup(LayoutWidget* InBody, float InWidth);
	bool IsGroupEditable() const;
	model::LibrarySource GetLibrarySource() const;
	void ChangeCampaign(const TFunction<void(model::CustomLibrary&), UiCallbackInlineSize>& InChange);
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
	void ChangeTileset(int InStep);
	void SyncTileset();
	LevelData Level;
	model::CustomLibrary Library;

	/** Unsaved edits of a level the user left; coming back restores the map and its undo history. */
	struct Draft
	{
		LevelData Level;
		model::EditorDocument Document;
		model::EditorHistory History;
	};
	TMap<FPersistentString, Draft> Drafts;
	std::string NewKey = "new:1";
	int NewCounter = 1;
	std::string LibraryGroup = "main";
	std::size_t LibraryPage = 0;
	float Zoom = 1.0f;
	bool bAutoZoom = true;
	TArray<LayoutWidget*> CampaignAddRows;
	std::size_t CampaignLevel = 0;
	bool bHasCurrent = false;
	model::LibraryEntry Current;
	model::EditorPalette Palette;
	model::EditorDocument Document;
	model::EditorHistory History;
	model::MapViewport Viewport;
	model::EditorLayoutResult Layout;
	model::EditorDockState Docks;
	TArray<std::string> ShownRows;

	cocos2d::ClippingRectangleNode* MapClip = nullptr;
	cocos2d::Node* MapContent = nullptr;
	cocos2d::DrawNode* MapGround = nullptr;
	cocos2d::DrawNode* MapChrome = nullptr;
	cocos2d::DrawNode* MinimapTiles = nullptr;
	cocos2d::Node* DocksLayer = nullptr;
	cocos2d::Node* MenuLayer = nullptr;
	cocos2d::DrawNode* SelectionMark = nullptr;
	model::EditorToolController Tools;
	TMap<long long, cocos2d::Node*> CellNodes;
	LayoutWidget* EnemyStepper = nullptr;
	std::array<LayoutWidget*, 3> DockHandles{};
	std::array<TArray<LayoutWidget*>, 3> DockTabs;
	TArray<std::pair<model::EditorMenuAction, LayoutWidget*>> MenuLines;
	TArray<LayoutWidget*> HistoryRows;
	TArray<LayoutWidget*> LibraryRows;

	TArray<LayoutWidget*> PaletteTiles;
	std::size_t LeftTab = 0;
	std::size_t RightTab = 0;
	TArray<std::size_t> TabEntries;
	std::size_t ActiveTab = 0;
	std::size_t SelectedEntry = 0;
	DragMode Drag = DragMode::None;
	bool bPanning = false;
	bool bFirstPoint = false;
	model::Vec2f PanFrom;
};
} // namespace game::view::cocos

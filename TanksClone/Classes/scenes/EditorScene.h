#pragma once

#include <string>
#include <vector>

#include "cocos2d.h"

#include "game/LevelLoader.h"
#include "model/EditorLayout.h"
#include "model/EditorPalette.h"
#include "scenes/TypedScene.h"
#include "services/ResourceHolder.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

/**
 * Paints the level map with the mouse and saves it as the custom level. The toolbar has a palette panel
 * with tabs (read from configs/editor/palette.json) and a separate panel of actions.
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

	void SelectTab(std::size_t InTab);
	void SelectItem(std::size_t InTabItem);

private:
	void LoadPalette();
	model::RectF GetToolbarBounds() const;
	void BuildPanels();
	void BuildActions(const model::EditorLayoutResult& InLayout);
	void BuildTabs(const model::EditorLayoutResult& InLayout);
	void BuildItems();
	cocos2d::Node* CreateTileNode(char InTile) const;
	cocos2d::Node* CreateIcon(const model::EditorPaletteEntry& InEntry, const cocos2d::Size& InBox) const;

	void PaintAt(const cocos2d::Vec2& InPoint);
	void SetCell(int InRow, int InColumn, char InTile);
	void RefreshCell(int InRow, int InColumn);
	void ResetMap();
	void ChangeEnemyCount(int InDelta);
	void UpdateEnemyLabel();

	// Returns true if the level is fine; otherwise shows what is wrong.
	bool CheckLevel();
	void Save();
	void Test();
	void ShowMessage(const std::string& InText, bool bInGood);

	ResourceHolder Atlases;
	std::string Tileset;
	LevelData Level;
	model::EditorPalette Palette;
	std::vector<cocos2d::Node*> CellNodes; // GridRows * GridColumns, nullptr for empty cells
	cocos2d::Node* Grid = nullptr;
	cocos2d::Node* ItemsLayer = nullptr;
	cocos2d::Label* EnemyLabel = nullptr;
	cocos2d::Size EnemyLabelBox;
	std::vector<widgets::Button*> TabButtons;
	std::vector<widgets::Button*> ItemButtons;
	std::vector<std::size_t> TabEntries; // palette entries shown on the active tab
	std::size_t ActiveTab = 0;
	std::size_t SelectedEntry = 0;
};

} // namespace tanks::view::cocos

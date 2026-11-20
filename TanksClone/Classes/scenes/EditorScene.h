#pragma once

#include <vector>
#include "cocos2d.h"
#include "scenes/TypedScene.h"
#include "game/LevelLoader.h"
#include "services/AtlasScope.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

// Paints the level map with the mouse and saves it as the custom level.
class EditorScene : public TypedScene<SceneKind::Editor>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(EditorScene);

private:
	void BuildToolbar();
	void SelectTile(size_t InPaletteIndex);
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

	AtlasScope Atlases;
	std::string Tileset;
	LevelData Level;
	std::vector<cocos2d::Node*> CellNodes; // GRID_ROWS * GRID_COLUMNS, nullptr for empty cells
	cocos2d::Node* Grid = nullptr;
	cocos2d::Label* EnemyLabel = nullptr;
	std::vector<widgets::Button*> PaletteButtons;
	size_t SelectedTile = 0;
};

} // namespace tanks::view::cocos
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cocos2d.h"

#include "scenes/TypedScene.h"
#include "services/ConfigRefs.h"
#include "viewmodel/WidgetEditSession.h"

namespace tanks::view::cocos
{

class LayoutWidget;

/**
 * The widget editor (debug builds only): opens a whole window (a root widget file with every widget nested in it),
 * lists its hierarchy on the left, selects any part by a click on the picture or on its row, steps its numbers up and
 * down with undo and redo, and saves every changed file back into the source tree. The numbers and the file text live
 * in viewmodel::WidgetEditSession, one per file; this scene only displays them. Names are shown as written, the tool is
 * not translated.
 */
class WidgetEditorScene : public TypedScene<SceneKind::WidgetEditor>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;
	void onExit() override;

	CREATE_FUNC(WidgetEditorScene);

private:
	// One line of the hierarchy: a child (or a file's own numbers) of the file that holds it.
	struct TreeRow
	{
		std::string File;
		int Index = viewmodel::WidgetEditSession::WidgetSelected;
		int Depth = 0;
		std::string Label;
		// Child ids of the nested widgets leading from the root widget to the one that holds File.
		std::vector<std::string> Chain;
	};

	void OpenRoot(int InIndex);
	void Rebuild();
	void BuildPicture();
	void BuildTree();
	void AppendRows(const std::string& InFile, int InDepth, const std::vector<std::string>& InChain);
	void BuildSidePanel();
	void BuildTopBar();
	void DrawSelection();
	bool PickAt(const cocos2d::Vec2& InPoint);
	bool PickIn(LayoutWidget* InOwner, const std::string& InFile, const std::vector<std::string>& InChain, const cocos2d::Vec2& InPoint, TreeRow& OutRow) const;
	void SelectRow(const TreeRow& InRow);
	LayoutWidget* FindOwner(const std::vector<std::string>& InChain) const;
	viewmodel::WidgetEditSession& GetSession(const std::string& InFile);
	viewmodel::WidgetEditSession& GetActive();
	bool IsAnyDirty() const;
	void Save();
	void SetMessage(const std::string& InText);

	std::vector<config_refs::GroupEntry> Files;
	int RootIndex = 0;
	int Zoom = 1;
	int TreeScroll = 0;
	std::string ActiveFile;
	std::vector<std::string> ActiveChain;
	std::map<std::string, std::unique_ptr<viewmodel::WidgetEditSession>> Sessions;
	std::vector<TreeRow> Rows;
	LayoutWidget* Picture = nullptr;
	cocos2d::Node* SidePanel = nullptr;
	cocos2d::Node* TopBar = nullptr;
	cocos2d::Node* Tree = nullptr;
	cocos2d::DrawNode* Selection = nullptr;
	cocos2d::Label* Message = nullptr;
};

} // namespace tanks::view::cocos

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cocos2d.h"

#include "scenes/TypedScene.h"
#include "services/ConfigRefs.h"
#include "templates/TFunction.h"
#include "viewmodel/MenuDef.h"
#include "viewmodel/MenuEditSession.h"
#include "viewmodel/ViewDef.h"
#include "viewmodel/WidgetEditSession.h"

namespace tanks::view::cocos
{
class LayoutWidget;

/**
 * The widget editor (debug builds only): lists the views (configs/ui/views: a window, a menu, a dialog), opens one as a whole window (a root widget file with every widget nested in it, filled with sample texts),
 * lists its hierarchy on the left, selects any part (or any menu button) by a click on the picture or on its row, steps its numbers up and
 * down, changes the fields of a menu button, reorders, copies and removes buttons, with undo and redo, and saves every changed file back into the source tree. The numbers and the file text live
 * in viewmodel::WidgetEditSession, one per file; this scene only displays them. Names are shown as written, the tool is
 * not translated.
 */
class WidgetEditorScene : public TypedScene<SceneKind::WidgetEditor>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;
	void onExit() override;
	bool ShowView(const std::string& InId);

	CREATE_FUNC(WidgetEditorScene);

private:
	/** One line of the hierarchy: a child (or a file's own numbers) of the file that holds it. */
	struct TreeRow
	{
		std::string File;
		int Index = viewmodel::WidgetEditSession::WidgetSelected;
		int Depth = 0;
		std::string Label;
		std::vector<std::string> Chain;
		int MenuItem = -1;
	};

	void ShowList();
	void OpenView(int InIndex);
	void ApplySamples();
	void ApplyMenu(const viewmodel::MenuDef& InMenu);
	void Rebuild();
	void BuildPicture();
	cocos2d::Rect GetViewportArea() const;
	void BuildTree();
	void AppendRows(const std::string& InFile, int InDepth, const std::vector<std::string>& InChain);
	void BuildSidePanel();
	void BuildMenuItemPanel();
	void OpenPicker(const std::string& InTitle, const std::string& InCurrent, const std::vector<std::string>& InChoices, bool bInPreview, const TFunction<void(const std::string&)>& InApply);
	std::vector<std::string> AllStringKeys() const;
	void ClosePicker();
	void BuildPicker();
	bool PickInPicker(const cocos2d::Vec2& InPoint);
	std::vector<std::string> ChoicesFor(viewmodel::MenuField InField) const;
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
	std::vector<viewmodel::ViewDef> Views;
	int ViewIndex = -1;
	int RootIndex = 0;
	int Zoom = 1;
	int TreeScroll = 0;
	std::string ActiveFile;
	std::vector<std::string> ActiveChain;
	std::map<std::string, std::unique_ptr<viewmodel::WidgetEditSession>> Sessions;
	std::map<std::string, std::unique_ptr<viewmodel::MenuEditSession>> MenuSessions;
	viewmodel::MenuEditSession* ActiveMenu = nullptr;
	std::vector<cocos2d::Node*> MenuButtons;
	cocos2d::Node* PickerNode = nullptr;
	std::string PickerTitle;
	std::string PickerCurrent;
	TFunction<void(const std::string&)> PickerApply;
	std::vector<std::pair<std::string, std::string>> PickerEntries;
	int PickerScroll = 0;
	std::vector<TreeRow> Rows;
	cocos2d::ClippingRectangleNode* Viewport = nullptr;
	LayoutWidget* Picture = nullptr;
	cocos2d::Node* SidePanel = nullptr;
	cocos2d::Node* TopBar = nullptr;
	cocos2d::Node* Tree = nullptr;
	cocos2d::Node* ListNode = nullptr;
	cocos2d::DrawNode* Selection = nullptr;
	cocos2d::Label* Message = nullptr;
};
} // namespace tanks::view::cocos

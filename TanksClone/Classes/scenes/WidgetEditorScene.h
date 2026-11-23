#pragma once

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
 * The widget editor (debug builds only): shows a widget file live, selects its parts, steps their numbers up and down
 * with undo and redo, and saves the file back into the source tree. The numbers and the file text live in
 * viewmodel::WidgetEditSession; this scene only displays them. Names are shown as written, the tool is not translated.
 */
class WidgetEditorScene : public TypedScene<SceneKind::WidgetEditor>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(WidgetEditorScene);

private:
	void OpenWidget(int InIndex);
	void Rebuild();
	void BuildPreview();
	void BuildSidePanel();
	void BuildTopBar();
	void DrawSelection();
	void Save();
	void SetMessage(const std::string& InText);

	std::vector<config_refs::GroupEntry> Files;
	int FileIndex = 0;
	int Zoom = 2;
	std::unique_ptr<viewmodel::WidgetEditSession> Session;
	LayoutWidget* Preview = nullptr;
	cocos2d::Node* SidePanel = nullptr;
	cocos2d::Node* TopBar = nullptr;
	cocos2d::DrawNode* Selection = nullptr;
	cocos2d::Label* Message = nullptr;
};

} // namespace tanks::view::cocos

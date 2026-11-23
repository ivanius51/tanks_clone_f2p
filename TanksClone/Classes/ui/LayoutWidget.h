#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "viewmodel/WidgetDef.h"

namespace tanks::view::cocos
{

/**
 * A widget built from a widget file (configs/ui/widgets/<name>.json) by the WidgetFactory: a node whose children are
 * labels, icons, rectangles, spacers and other widgets, placed by the layout rules of the file (row, column, anchor,
 * stack). Its origin is its centre. Whenever a text, a frame or the size changes, the widget measures its children and
 * places them again, and so does every widget around it, so nothing stays at a stale position.
 */
class LayoutWidget : public cocos2d::Node
{
public:
	using CommandHandler = std::function<void(std::string_view)>;

	// nullptr (and logged) when no widget file has this id.
	static LayoutWidget* Create(std::string_view InId);
	static LayoutWidget* Create(const viewmodel::WidgetDef& InDef);

	// The child's text; an unknown child is ignored.
	void SetText(std::string_view InChild, const std::string& InText);
	std::string GetText(std::string_view InChild) const;
	void SetTextColor(std::string_view InChild, const cocos2d::Color3B& InColor);
	void SetIconFrame(std::string_view InChild, const std::string& InFrame);
	// Replaces the file's "size" (0 on an axis = as big as the content).
	void SetFixedSize(const cocos2d::Size& InSize);

	cocos2d::Label* FindLabel(std::string_view InChild) const;
	cocos2d::Node* FindPart(std::string_view InChild) const;
	LayoutWidget* FindWidget(std::string_view InChild) const;

	// Builds every child again (after the font, the language or the text size changed), keeping set texts.
	void Rebuild();
	// Measures and places the children again, here and in every widget around this one.
	void Relayout();

	const viewmodel::WidgetDef& GetDef() const;
	// Runs the owner's handler with the file's on_click command id; false when there is none.
	bool Activate() const;
	void SetCommandHandler(const CommandHandler& InHandler);

private:
	struct Part
	{
		viewmodel::WidgetChildDef Def;
		cocos2d::Node* Node = nullptr;
		cocos2d::Label* TextLabel = nullptr;
		cocos2d::DrawNode* Drawing = nullptr;
		cocos2d::Sprite* Picture = nullptr;
		LayoutWidget* Sub = nullptr;
		cocos2d::Size Size;
	};

	bool init(const viewmodel::WidgetDef& InDef);
	void Build();
	void BuildPart(const viewmodel::WidgetChildDef& InDef);
	Part* FindEntry(std::string_view InChild);
	const Part* FindEntry(std::string_view InChild) const;
	LayoutWidget* GetRoot();
	cocos2d::Size Arrange();
	cocos2d::Size Measure(Part& InOutPart, float InAvailableWidth) const;
	void Resize(Part& InOutPart, const cocos2d::Size& InSize);
	void DrawRect(Part& InOutPart);
	void DrawRing(Part& InOutPart);
	float GetScale() const;

	viewmodel::WidgetDef Def;
	std::vector<Part> Parts;
	std::map<std::string, std::string> Texts;
	LayoutWidget* Outer = nullptr;
	cocos2d::Size FixedOverride;
	CommandHandler Handler;
};

} // namespace tanks::view::cocos

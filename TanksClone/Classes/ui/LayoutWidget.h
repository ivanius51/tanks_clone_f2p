#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "model/Animation.h"
#include "model/BarMeter.h"
#include "templates/TFunction.h"
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
	using CommandHandler = TFunction<void(std::string_view)>;

	static LayoutWidget* Create(std::string_view InId);
	static LayoutWidget* Create(const viewmodel::WidgetDef& InDef);

	void SetText(std::string_view InChild, const std::string& InText);
	std::string GetText(std::string_view InChild) const;
	void SetTextColor(std::string_view InChild, const cocos2d::Color3B& InColor);
	void SetIconFrame(std::string_view InChild, const std::string& InFrame);
	void SetFixedSize(const cocos2d::Size& InSize);
	float GetInnerWidth() const;
	float GetGap() const;
	void SetGap(float InGap);
	void SetPartOpacity(std::string_view InChild, float InOpacity);
	void SetPartColors(std::string_view InChild, std::string_view InFill, std::string_view InBorder);
	void SetPartFraction(std::string_view InChild, float InFraction);
	float GetPartShownFraction(std::string_view InChild) const;
	void SetPartFillColor(std::string_view InChild, const model::Color& InColor);
	void SetPartVisible(std::string_view InChild, bool bInVisible);
	void PlayPartAnimation(std::string_view InChild, std::string_view InAnimation);
	void SetHovered(bool bInHovered);
	bool IsHovered() const;
	bool IsHoverable() const;
	int GetHoverLayer() const;
	model::RectF GetWorldBounds() const;

	void onEnter() override;
	void onExit() override;

	void AppendNode(cocos2d::Node* InNode);
	void RemoveAppendedNodes();

	cocos2d::Label* FindLabel(std::string_view InChild) const;
	cocos2d::Node* FindPart(std::string_view InChild) const;
	LayoutWidget* FindWidget(std::string_view InChild) const;

	void update(float InDelta) override;

	static void ReloadAll();

	void Rebuild();
	void Relayout();

	const viewmodel::WidgetDef& GetDef() const;
	bool Activate() const;
	void SetCommandHandler(const CommandHandler& InHandler);

private:
	struct Part
	{
		viewmodel::WidgetChildDef Def;
		cocos2d::Node* Node = nullptr;
		cocos2d::Label* TextLabel = nullptr;
		std::string FullText;
		cocos2d::DrawNode* Drawing = nullptr;
		cocos2d::Sprite* Picture = nullptr;
		LayoutWidget* Sub = nullptr;
		cocos2d::Size Size;
		model::BarMeter Meter;
		bool bAppended = false;
		std::optional<model::AnimationPlayer> Player;
		cocos2d::Vec2 LayoutPosition;
		float BaseScaleX = 1.0f;
		float BaseScaleY = 1.0f;
		bool bHovered = false;
	};

	void ShowLabelText(Part& InOutPart, const std::string& InText);
	LayoutWidget* CreateRichRow(const viewmodel::WidgetChildDef& InDef, const std::string& InText) const;
	void ApplyAnimation(Part& InOutPart);
	void StartAnimation(Part& InOutPart, std::string_view InAnimation);
	const viewmodel::ColorRef& FillOf(const Part& InPart) const;
	const viewmodel::ColorRef& BorderOf(const Part& InPart) const;

	static bool ShrinksText(const Part& InPart);
	void FitText(Part& InOutPart, float InAvailableWidth) const;

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
	void DrawBar(Part& InOutPart);
	float GetScale() const;

	viewmodel::WidgetDef Def;
	std::vector<Part> Parts;
	std::map<std::string, std::string> Texts;
	LayoutWidget* Outer = nullptr;
	~LayoutWidget() override;
	cocos2d::Size FixedOverride;
	float GapOverride = -1.0f;
	cocos2d::Vector<cocos2d::Node*> Appended;
	CommandHandler Handler;
	bool bHovered = false;
};
} // namespace tanks::view::cocos

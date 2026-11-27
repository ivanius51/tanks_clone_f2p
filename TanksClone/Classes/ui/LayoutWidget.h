#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "cocos2d.h"

#include "model/Animation.h"
#include "model/BarMeter.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "viewmodel/WidgetDef.h"

namespace game::view::cocos
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
	using CommandHandler = TFunction<void(std::string_view), UiCallbackInlineSize>;
	using ClickAction = TFunction<void(), UiCallbackInlineSize>;
	using ValueAction = TFunction<void(float), UiCallbackInlineSize>;

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
	void SetPressed(bool bInPressed);
	bool IsHovered() const;
	bool IsHoverable() const;
	int GetHoverLayer() const;
	model::RectF GetWorldBounds() const;

	void onEnter() override;
	void onExit() override;

	void AppendNode(cocos2d::Node* InNode);
	void AppendWidget(LayoutWidget* InWidget, bool bInStretch);
	TArray<LayoutWidget*> SetListCount(std::string_view InChild, std::size_t InCount);
	void SetFlag(std::string_view InFlag, bool bInValue);
	LayoutWidget* FindWidgetPath(std::string_view InPath) const;
	void SetTextAtPath(std::string_view InPath, const std::string& InText);
	void SetClickHandler(const ClickAction& InAction);
	void BindClick(std::string_view InPath, const ClickAction& InAction);
	void SetEnabled(bool bInEnabled);
	bool IsEnabled() const { return bEnabled; }
	bool HasClickHandler() const { return static_cast<bool>(OnClickAction); }
	bool Click() const;
	void SetSliderValue(float InValue);
	float GetSliderValue() const { return SliderValue; }
	void SetSliderHandler(const ValueAction& InAction);
	void RemoveAppendedNodes();

	cocos2d::Label* FindLabel(std::string_view InChild) const;
	cocos2d::Node* FindPart(std::string_view InChild) const;
	LayoutWidget* FindWidget(std::string_view InChild) const;

	void update(float InDelta) override;
	void visit(cocos2d::Renderer* InRenderer, const cocos2d::Mat4& InParentTransform, uint32_t InParentFlags) override;

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
		bool bPressed = false;
		cocos2d::RefPtr<cocos2d::SpriteFrame> TileFrame;
	};

	void ApplyFlags();
	bool IsShownByFlags(const viewmodel::WidgetChildDef& InDef) const;
	void ApplySubTexts(Part& InOutPart);
	bool IsInsideTouch(const cocos2d::Touch* InTouch) const;
	bool IsShownInScene() const;
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
	void FitImage(Part& InOutPart, const cocos2d::Size& InSize);
	void DrawRect(Part& InOutPart);
	void DrawRing(Part& InOutPart);
	void DrawGlyph(Part& InOutPart);
	void DrawTiles(Part& InOutPart);
	void ApplySlider();
	void DragSlider(const cocos2d::Touch* InTouch);
	void DrawBar(Part& InOutPart);
	float GetScale() const;

	viewmodel::WidgetDef Def;
	TArray<Part> Parts;
	TMap<FPersistentString, std::string> Texts;
	LayoutWidget* Outer = nullptr;
	~LayoutWidget() override;
	cocos2d::Size FixedOverride;
	float GapOverride = -1.0f;
	cocos2d::Vector<cocos2d::Node*> Appended;
	TMap<FPersistentString, bool> Flags;
	ClickAction OnClickAction;
	bool bEnabled = true;
	float SliderValue = 0.0f;
	ValueAction OnSliderChanged;
	bool bDraggingSlider = false;
	cocos2d::CustomCommand ClipBegin;
	cocos2d::CustomCommand ClipEnd;
	model::RectF ClipBox;
	CommandHandler Handler;
	bool bHovered = false;
	bool bPressed = false;
};
} // namespace game::view::cocos

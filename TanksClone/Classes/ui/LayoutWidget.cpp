#include "ui/LayoutWidget.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

#include "core/GameContext.h"
#include "model/WidgetLayout.h"
#include "services/ResourceManager.h"
#include "model/RichText.h"
#include "services/HoverManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/CocosConvert.h"
#include "ui/Widgets.h"

namespace
{
	size_t ByteOffsetOfCharacter(const std::string& InText, const size_t InCharacters)
	{
		size_t Offset = 0;
		size_t Count = 0;
		while ( Offset < InText.size() && Count < InCharacters )
		{
			++Offset;
			while ( Offset < InText.size() && (static_cast<unsigned char>(InText[Offset]) & 0xC0) == 0x80 )
			{
				++Offset;
			}
			++Count;
		}
		return Offset;
	}

	size_t CountCharacters(const std::string& InText)
	{
		return static_cast<size_t>(std::count_if(InText.begin(), InText.end(), [](const char InByte) { return (static_cast<unsigned char>(InByte) & 0xC0) != 0x80; }));
	}

	std::string CutWithEllipsis(const std::string& InText, const size_t InCharacters)
	{
		if ( CountCharacters(InText) <= InCharacters )
		{
			return InText;
		}
		return InText.substr(0, ByteOffsetOfCharacter(InText, InCharacters)) + "...";
	}
}

namespace game::view::cocos
{

namespace
{
	bool HasIconMarkup(const std::string& InText)
	{
		const std::vector<model::TextRun> Runs = model::SplitIconMarkup(InText);
		return std::any_of(Runs.begin(), Runs.end(), [](const model::TextRun& InRun) { return InRun.bIcon; });
	}

	const viewmodel::WidgetColors& Palette()
	{
		return GameContext::Get<UiStyle>().GetWidgets().Colors;
	}

	std::optional<model::Color> Resolve(const viewmodel::ColorRef& InColor)
	{
		if ( InColor.bLiteral )
		{
			return InColor.Literal;
		}
		const model::Color* const Found = Palette().Find(InColor.Name);
		return Found ? std::optional<model::Color>(*Found) : std::nullopt;
	}

	cocos2d::Color3B TextColorOf(const viewmodel::ColorRef& InColor)
	{
		const std::optional<model::Color> Found = Resolve(InColor);
		return Found ? ToCocos3B(*Found) : widgets::colors::TextNormal();
	}

	void FillRounded(cocos2d::DrawNode* const InNode, const cocos2d::Size& InHalf, const float InRadius, const cocos2d::Color4F& InColor)
	{
		const float Radius = std::clamp(InRadius, 0.0f, std::min(InHalf.width, InHalf.height));
		InNode->drawSolidRect(cocos2d::Vec2(-InHalf.width + Radius, -InHalf.height), cocos2d::Vec2(InHalf.width - Radius, InHalf.height), InColor);
		InNode->drawSolidRect(cocos2d::Vec2(-InHalf.width, -InHalf.height + Radius), cocos2d::Vec2(InHalf.width, InHalf.height - Radius), InColor);
		if ( Radius <= 0.0f )
		{
			return;
		}
		constexpr int CornerSegments = 16;
		for ( const float SignX : { -1.0f, 1.0f } )
		{
			for ( const float SignY : { -1.0f, 1.0f } )
			{
				InNode->drawSolidCircle(cocos2d::Vec2(SignX * (InHalf.width - Radius), SignY * (InHalf.height - Radius)), Radius, 0.0f, CornerSegments, InColor);
			}
		}
	}

	model::Vec2f ToModel(const cocos2d::Size& InSize)
	{
		return { InSize.width, InSize.height };
	}

	cocos2d::Size ToSize(const model::Vec2f& InSize)
	{
		return { InSize.X, InSize.Y };
	}
}

namespace
{
	std::vector<LayoutWidget*> GLiveRoots;
}

LayoutWidget::~LayoutWidget()
{
	GLiveRoots.erase(std::remove(GLiveRoots.begin(), GLiveRoots.end(), this), GLiveRoots.end());
}

void LayoutWidget::ReloadAll()
{
	const UiStyle& Style = GameContext::Get<UiStyle>();
	const std::vector<LayoutWidget*> Roots = GLiveRoots;
	for ( LayoutWidget* const Current : Roots )
	{
		if ( std::find(GLiveRoots.begin(), GLiveRoots.end(), Current) == GLiveRoots.end() )
		{
			continue;
		}
		const viewmodel::WidgetDef* const Fresh = Current->Outer || Current->Def.Id.empty() ? nullptr : Style.FindWidget(Current->Def.Id);
		if ( Fresh )
		{
			Current->Def = *Fresh;
			Current->Rebuild();
		}
	}
}

LayoutWidget* LayoutWidget::Create(const std::string_view InId)
{
	const viewmodel::WidgetDef* const Found = GameContext::Get<UiStyle>().FindWidget(InId);
	return Found ? Create(*Found) : nullptr;
}

LayoutWidget* LayoutWidget::Create(const viewmodel::WidgetDef& InDef)
{
	LayoutWidget* NewWidget = new (std::nothrow) LayoutWidget();
	if ( NewWidget && NewWidget->init(InDef) )
	{
		NewWidget->autorelease();
		return NewWidget;
	}
	CC_SAFE_DELETE(NewWidget);
	return nullptr;
}

bool LayoutWidget::init(const viewmodel::WidgetDef& InDef)
{
	if ( !Node::init() )
	{
		return false;
	}
	Def = InDef;
	GLiveRoots.push_back(this);
	Build();
	Arrange();
	return true;
}

float LayoutWidget::GetGap() const
{
	return (GapOverride >= 0.0f ? GapOverride : Def.Rules.Gap) * GetScale();
}

float LayoutWidget::GetInnerWidth() const
{
	const float Scale = GetScale();
	const float Fixed = FixedOverride.width > 0.0f ? FixedOverride.width : Def.Rules.FixedSize.X * Scale;
	return std::max(0.0f, Fixed - 2.0f * Def.Rules.Padding.X * Scale);
}

float LayoutWidget::GetScale() const
{
	return Def.bScaleWithText ? widgets::GetUiScale() : 1.0f;
}

void LayoutWidget::Build()
{
	Parts.reserve(Def.Children.size());
	for ( const viewmodel::WidgetChildDef& Current : Def.Children )
	{
		BuildPart(Current);
	}
}

void LayoutWidget::BuildPart(const viewmodel::WidgetChildDef& InDef)
{
	Part NewPart;
	NewPart.Def = InDef;
	switch ( InDef.Part )
	{
		case viewmodel::WidgetPart::Label:
		{
			const std::string Text = !InDef.TextKey.empty() ? GameContext::Get<StringsManager>().GetString(InDef.TextKey) : InDef.Text;
			NewPart.FullText = Text;
			NewPart.Sub = HasIconMarkup(Text) ? CreateRichRow(InDef, Text) : nullptr;
			if ( NewPart.Sub )
			{
				NewPart.Node = NewPart.Sub;
				break;
			}
			NewPart.TextLabel = widgets::CreateLabel(InDef.MaxChars > 0 ? CutWithEllipsis(Text, static_cast<size_t>(InDef.MaxChars)) : Text, InDef.FontSize, TextColorOf(InDef.TextColor), InDef.bDecorative);
			NewPart.Node = NewPart.TextLabel;
			break;
		}
		case viewmodel::WidgetPart::Icon:
			NewPart.Picture = InDef.Frame.empty() ? cocos2d::Sprite::create() : widgets::CreateFrameSprite(InDef.Frame);
			NewPart.Node = NewPart.Picture ? static_cast<cocos2d::Node*>(NewPart.Picture) : cocos2d::Node::create();
			break;
		case viewmodel::WidgetPart::Rect:
		case viewmodel::WidgetPart::Ring:
		case viewmodel::WidgetPart::Bar:
			NewPart.Meter = model::BarMeter(InDef.AnimTime, InDef.TrailDelay, InDef.AnimEase);
			NewPart.Drawing = cocos2d::DrawNode::create();
			NewPart.Node = NewPart.Drawing;
			break;
		case viewmodel::WidgetPart::Widget:
			NewPart.Sub = Create(InDef.Widget);
			NewPart.Node = NewPart.Sub ? static_cast<cocos2d::Node*>(NewPart.Sub) : cocos2d::Node::create();
			break;
		default:
			NewPart.Node = cocos2d::Node::create();
			break;
	}
	if ( NewPart.Sub )
	{
		NewPart.Sub->Outer = this;
	}
	NewPart.Node->setCascadeOpacityEnabled(true);
	NewPart.Node->setOpacity(static_cast<GLubyte>(std::clamp(InDef.Opacity, 0.0f, 1.0f) * 255.0f));
	NewPart.Node->setVisible(InDef.bVisible);
	if ( !InDef.Animation.empty() )
	{
		StartAnimation(NewPart, InDef.Animation);
	}
	NewPart.Node->setRotation(InDef.Rotation);
	if ( !NewPart.TextLabel && !NewPart.Picture )
	{
		NewPart.Node->setScale(InDef.Scale);
	}
	addChild(NewPart.Node, InDef.ZOrder * 1000 + static_cast<int>(Parts.size()));
	Parts.push_back(std::move(NewPart));
}

const viewmodel::ColorRef& LayoutWidget::FillOf(const Part& InPart) const
{
	return InPart.bHovered && InPart.Def.HoverFill.IsSet() ? InPart.Def.HoverFill : InPart.Def.FillColor;
}

const viewmodel::ColorRef& LayoutWidget::BorderOf(const Part& InPart) const
{
	return InPart.bHovered && InPart.Def.HoverBorder.IsSet() ? InPart.Def.HoverBorder : InPart.Def.BorderColor;
}

void LayoutWidget::ApplyAnimation(Part& InOutPart)
{
	const model::AnimationValues Values = InOutPart.Player->Sample();
	InOutPart.Node->setPosition(InOutPart.LayoutPosition + cocos2d::Vec2(Values.MoveX, Values.MoveY));
	InOutPart.Node->setScale(InOutPart.BaseScaleX * Values.ScaleX, InOutPart.BaseScaleY * Values.ScaleY);
	InOutPart.Node->setRotation(InOutPart.Def.Rotation + Values.Rotation);
	InOutPart.Node->setOpacity(static_cast<GLubyte>(std::clamp(InOutPart.Def.Opacity * Values.Alpha, 0.0f, 1.0f) * 255.0f));
}

void LayoutWidget::StartAnimation(Part& InOutPart, const std::string_view InAnimation)
{
	InOutPart.Player.reset();
	const model::AnimationDef* const Found = InAnimation.empty() ? nullptr : GameContext::Get<UiStyle>().FindAnimation(InAnimation);
	if ( Found )
	{
		InOutPart.Player.emplace(*Found);
		InOutPart.BaseScaleX = InOutPart.Node->getScaleX();
		InOutPart.BaseScaleY = InOutPart.Node->getScaleY();
		scheduleUpdate();
	}
}

void LayoutWidget::PlayPartAnimation(const std::string_view InChild, const std::string_view InAnimation)
{
	if ( Part* const Entry = FindEntry(InChild) )
	{
		StartAnimation(*Entry, InAnimation);
		Relayout();
	}
}

bool LayoutWidget::IsHovered() const
{
	return bHovered;
}

int LayoutWidget::GetHoverLayer() const
{
	return Def.Layer;
}

bool LayoutWidget::IsHoverable() const
{
	if ( !Def.OnClick.empty() )
	{
		return true;
	}
	return std::any_of(Parts.begin(), Parts.end(), [](const Part& InPart) { return InPart.Def.HoverFill.IsSet() || InPart.Def.HoverBorder.IsSet() || InPart.Def.HoverOpacity >= 0.0f; });
}

void LayoutWidget::SetHovered(const bool bInHovered)
{
	if ( bHovered == bInHovered )
	{
		return;
	}
	bHovered = bInHovered;
	for ( Part& Current : Parts )
	{
		Current.bHovered = bInHovered;
		if ( Current.Def.HoverFill.IsSet() || Current.Def.HoverBorder.IsSet() )
		{
			Resize(Current, Current.Size);
		}
		if ( Current.Def.HoverOpacity >= 0.0f && !Current.Player )
		{
			Current.Node->setOpacity(static_cast<GLubyte>(std::clamp(bInHovered ? Current.Def.HoverOpacity : Current.Def.Opacity, 0.0f, 1.0f) * 255.0f));
		}
		if ( Current.Sub )
		{
			Current.Sub->SetHovered(bInHovered);
		}
	}
}

model::RectF LayoutWidget::GetWorldBounds() const
{
	const cocos2d::Size Box = getContentSize();
	const cocos2d::Vec2 A = convertToWorldSpace(cocos2d::Vec2(-Box.width / 2.0f, -Box.height / 2.0f));
	const cocos2d::Vec2 B = convertToWorldSpace(cocos2d::Vec2(Box.width / 2.0f, Box.height / 2.0f));
	return model::RectF(std::min(A.x, B.x), std::min(A.y, B.y), std::abs(B.x - A.x), std::abs(B.y - A.y));
}

void LayoutWidget::onEnter()
{
	Node::onEnter();
	if ( !Outer && IsHoverable() )
	{
		GameContext::Get<HoverManager>().Register(this);
	}
}

void LayoutWidget::onExit()
{
	if ( !Outer )
	{
		GameContext::Get<HoverManager>().Unregister(this);
	}
	SetHovered(false);
	Node::onExit();
}

LayoutWidget::Part* LayoutWidget::FindEntry(const std::string_view InChild)
{
	const auto Found = std::find_if(Parts.begin(), Parts.end(), [InChild](const Part& InPart) { return InPart.Def.Id == InChild; });
	return Found == Parts.end() ? nullptr : &*Found;
}

const LayoutWidget::Part* LayoutWidget::FindEntry(const std::string_view InChild) const
{
	return const_cast<LayoutWidget*>(this)->FindEntry(InChild);
}

void LayoutWidget::SetText(const std::string_view InChild, const std::string& InText)
{
	Part* const Entry = FindEntry(InChild);
	if ( !Entry || Entry->Def.Part != viewmodel::WidgetPart::Label )
	{
		return;
	}
	Texts[std::string(InChild)] = InText;
	if ( Entry->FullText == InText )
	{
		return;
	}
	Entry->FullText = InText;
	if ( Entry->Sub || HasIconMarkup(InText) )
	{
		ShowLabelText(*Entry, InText);
	}
	else
	{
		Entry->TextLabel->setString(Entry->Def.MaxChars > 0 ? CutWithEllipsis(InText, static_cast<size_t>(Entry->Def.MaxChars)) : InText);
	}
	Relayout();
}

LayoutWidget* LayoutWidget::CreateRichRow(const viewmodel::WidgetChildDef& InDef, const std::string& InText) const
{
	viewmodel::WidgetDef Row;
	Row.Rules.Kind = model::LayoutKind::Row;
	Row.bScaleWithText = true;
	for ( const model::TextRun& Run : model::SplitIconMarkup(InText) )
	{
		viewmodel::WidgetChildDef Piece = InDef;
		Piece.Id.clear();
		if ( Run.bIcon )
		{
			Piece.Part = viewmodel::WidgetPart::Icon;
			Piece.Frame = Run.Value;
			Piece.Size = model::Vec2f(InDef.FontSize, InDef.FontSize);
		}
		else
		{
			Piece.Text = Run.Value;
			Piece.TextKey.clear();
			Piece.MaxChars = 0;
			Piece.MaxWidth = 0.0f;
			Piece.bShrinkToFit = false;
			Piece.bWrap = false;
		}
		Row.Children.push_back(Piece);
	}
	LayoutWidget* const Result = Create(Row);
	if ( Result )
	{
		Result->Outer = const_cast<LayoutWidget*>(this);
	}
	return Result;
}

void LayoutWidget::ShowLabelText(Part& InOutPart, const std::string& InText)
{
	LayoutWidget* const Rich = HasIconMarkup(InText) ? CreateRichRow(InOutPart.Def, InText) : nullptr;
	cocos2d::Label* const Plain = Rich ? nullptr : widgets::CreateLabel(InOutPart.Def.MaxChars > 0 ? CutWithEllipsis(InText, static_cast<size_t>(InOutPart.Def.MaxChars)) : InText, InOutPart.Def.FontSize, TextColorOf(InOutPart.Def.TextColor), InOutPart.Def.bDecorative);
	cocos2d::Node* const Fresh = Rich ? static_cast<cocos2d::Node*>(Rich) : Plain;
	Fresh->setCascadeOpacityEnabled(true);
	Fresh->setVisible(InOutPart.Node->isVisible());
	Fresh->setOpacity(InOutPart.Node->getOpacity());
	const int ZOrder = InOutPart.Node->getLocalZOrder();
	InOutPart.Node->removeFromParent();
	addChild(Fresh, ZOrder);
	InOutPart.Node = Fresh;
	InOutPart.Sub = Rich;
	InOutPart.TextLabel = Plain;
}

std::string LayoutWidget::GetText(const std::string_view InChild) const
{
	const Part* const Entry = FindEntry(InChild);
	if ( Entry && Entry->Sub && Entry->Def.Part == viewmodel::WidgetPart::Label )
	{
		return Entry->FullText;
	}
	return Entry && Entry->TextLabel ? Entry->TextLabel->getString() : std::string();
}

void LayoutWidget::SetTextColor(const std::string_view InChild, const cocos2d::Color3B& InColor)
{
	if ( Part* const Entry = FindEntry(InChild); Entry && Entry->TextLabel )
	{
		Entry->TextLabel->setColor(InColor);
	}
}

void LayoutWidget::SetIconFrame(const std::string_view InChild, const std::string& InFrame)
{
	Part* const Entry = FindEntry(InChild);
	if ( !Entry || Entry->Def.Part != viewmodel::WidgetPart::Icon || !Entry->Picture )
	{
		return;
	}
	cocos2d::SpriteFrame* const Frame = GameContext::Get<ResourceManager>().FindFrame(InFrame);
	if ( Frame )
	{
		Entry->Picture->setSpriteFrame(Frame);
		Relayout();
	}
}

void LayoutWidget::SetFixedSize(const cocos2d::Size& InSize)
{
	FixedOverride = InSize;
	Relayout();
}

cocos2d::Label* LayoutWidget::FindLabel(const std::string_view InChild) const
{
	const Part* const Entry = FindEntry(InChild);
	return Entry ? Entry->TextLabel : nullptr;
}

cocos2d::Node* LayoutWidget::FindPart(const std::string_view InChild) const
{
	const Part* const Entry = FindEntry(InChild);
	return Entry ? Entry->Node : nullptr;
}

LayoutWidget* LayoutWidget::FindWidget(const std::string_view InChild) const
{
	const Part* const Entry = FindEntry(InChild);
	return Entry ? Entry->Sub : nullptr;
}

void LayoutWidget::SetGap(const float InGap)
{
	GapOverride = InGap;
	Relayout();
}

void LayoutWidget::SetPartOpacity(const std::string_view InChild, const float InOpacity)
{
	if ( Part* const Entry = FindEntry(InChild) )
	{
		Entry->Def.Opacity = std::clamp(InOpacity, 0.0f, 1.0f);
		Entry->Node->setOpacity(static_cast<GLubyte>(Entry->Def.Opacity * 255.0f));
	}
}

void LayoutWidget::SetPartColors(const std::string_view InChild, const std::string_view InFill, const std::string_view InBorder)
{
	Part* const Entry = FindEntry(InChild);
	if ( !Entry || !Entry->Drawing )
	{
		return;
	}
	if ( !InFill.empty() )
	{
		Entry->Def.FillColor = viewmodel::ColorRef(std::string(InFill));
	}
	if ( !InBorder.empty() )
	{
		Entry->Def.BorderColor = viewmodel::ColorRef(std::string(InBorder));
	}
	Resize(*Entry, Entry->Size);
}

void LayoutWidget::SetPartFraction(const std::string_view InChild, const float InFraction)
{
	Part* const Entry = FindEntry(InChild);
	if ( !Entry || Entry->Def.Part != viewmodel::WidgetPart::Bar )
	{
		return;
	}
	Entry->Meter.SetTarget(InFraction);
	Resize(*Entry, Entry->Size);
	if ( !Entry->Meter.IsSettled() )
	{
		scheduleUpdate();
	}
}

float LayoutWidget::GetPartShownFraction(const std::string_view InChild) const
{
	const Part* const Entry = FindEntry(InChild);
	return Entry ? Entry->Meter.GetShown() : 0.0f;
}

void LayoutWidget::update(const float InDelta)
{
	Node::update(InDelta);
	bool bAnyMoving = false;
	for ( Part& Current : Parts )
	{
		if ( Current.Player && !Current.Player->IsFinished() )
		{
			Current.Player->Advance(InDelta);
			ApplyAnimation(Current);
			bAnyMoving = bAnyMoving || !Current.Player->IsFinished();
		}
		if ( Current.Def.Part != viewmodel::WidgetPart::Bar || Current.Meter.IsSettled() )
		{
			continue;
		}
		Current.Meter.Advance(InDelta);
		Resize(Current, Current.Size);
		bAnyMoving = bAnyMoving || !Current.Meter.IsSettled();
	}
	if ( !bAnyMoving )
	{
		unscheduleUpdate();
	}
}

void LayoutWidget::SetPartFillColor(const std::string_view InChild, const model::Color& InColor)
{
	Part* const Entry = FindEntry(InChild);
	if ( !Entry || !Entry->Drawing )
	{
		return;
	}
	Entry->Def.FillColor.Literal = InColor;
	Entry->Def.FillColor.bLiteral = true;
	Resize(*Entry, Entry->Size);
}

void LayoutWidget::SetPartVisible(const std::string_view InChild, const bool bInVisible)
{
	Part* const Entry = FindEntry(InChild);
	if ( Entry && Entry->Node->isVisible() != bInVisible )
	{
		Entry->Node->setVisible(bInVisible);
		Relayout();
	}
}

void LayoutWidget::AppendNode(cocos2d::Node* const InNode)
{
	Part NewPart;
	NewPart.Node = InNode;
	NewPart.bAppended = true;
	addChild(InNode, static_cast<int>(Parts.size()));
	Parts.push_back(std::move(NewPart));
	Appended.pushBack(InNode);
	Relayout();
}

void LayoutWidget::RemoveAppendedNodes()
{
	for ( cocos2d::Node* const Current : Appended )
	{
		Current->removeFromParent();
	}
	Appended.clear();
	Parts.erase(std::remove_if(Parts.begin(), Parts.end(), [](const Part& InPart) { return InPart.bAppended; }), Parts.end());
	Relayout();
}

void LayoutWidget::Rebuild()
{
	removeAllChildren();
	Parts.clear();
	Build();
	for ( cocos2d::Node* const Current : Appended )
	{
		Part NewPart;
		NewPart.Node = Current;
		NewPart.bAppended = true;
		addChild(Current, static_cast<int>(Parts.size()));
		Parts.push_back(std::move(NewPart));
	}
	for ( const auto& [Child, Text] : Texts )
	{
		if ( Part* const Entry = FindEntry(Child); Entry && Entry->TextLabel )
		{
			Entry->FullText = Text;
			Entry->TextLabel->setString(Entry->Def.MaxChars > 0 ? CutWithEllipsis(Text, static_cast<size_t>(Entry->Def.MaxChars)) : Text);
		}
	}
	Relayout();
}

LayoutWidget* LayoutWidget::GetRoot()
{
	LayoutWidget* Root = this;
	while ( Root->Outer )
	{
		Root = Root->Outer;
	}
	return Root;
}

void LayoutWidget::Relayout()
{
	GetRoot()->Arrange();
}

cocos2d::Size LayoutWidget::Measure(Part& InOutPart, const float InAvailableWidth) const
{
	const float Scale = GetScale();
	const viewmodel::WidgetChildDef& Child = InOutPart.Def;
	if ( InOutPart.bAppended )
	{
		return InOutPart.Node->getContentSize();
	}
	if ( InOutPart.TextLabel )
	{
		const float Limit = Child.MaxWidth > 0.0f ? (InAvailableWidth > 0.0f ? std::min(InAvailableWidth, Child.MaxWidth * Scale) : Child.MaxWidth * Scale) : InAvailableWidth;
		if ( Child.bWrap && Limit > 0.0f )
		{
			InOutPart.TextLabel->setScale(1.0f);
			InOutPart.TextLabel->setAlignment(cocos2d::TextHAlignment::CENTER);
			InOutPart.TextLabel->setDimensions(Limit, 0.0f);
		}
		FitText(InOutPart, Limit);
		return InOutPart.TextLabel->getContentSize() * InOutPart.TextLabel->getScale();
	}
	if ( InOutPart.Picture )
	{
		const cocos2d::Size Natural = InOutPart.Picture->getContentSize();
		const cocos2d::Size Box = ToSize(Child.Size) * Scale;
		const bool bFits = Box.width > 0.0f && Box.height > 0.0f && Natural.width > 0.0f && Natural.height > 0.0f;
		const float Fit = bFits ? std::min(Box.width / Natural.width, Box.height / Natural.height) : 1.0f;
		InOutPart.Picture->setScale(Fit);
		return Natural * Fit;
	}
	if ( InOutPart.Sub )
	{
		if ( Child.Size.X > 0.0f || Child.Size.Y > 0.0f || Child.bStretch || Child.Grow > 0.0f )
		{
			InOutPart.Sub->FixedOverride = ToSize(Child.Size) * Scale;
		}
		return InOutPart.Sub->Arrange();
	}
	return ToSize(Child.Size) * Scale;
}

bool LayoutWidget::ShrinksText(const Part& InPart)
{
	return InPart.TextLabel && (InPart.Def.bShrinkToFit || InPart.Def.MaxWidth > 0.0f);
}

void LayoutWidget::FitText(Part& InOutPart, const float InLimit) const
{
	cocos2d::Label* const Label = InOutPart.TextLabel;
	const viewmodel::WidgetChildDef& Child = InOutPart.Def;
	const std::string Wanted = Child.MaxChars > 0 ? CutWithEllipsis(InOutPart.FullText, static_cast<size_t>(Child.MaxChars)) : InOutPart.FullText;
	if ( Label->getString() != Wanted )
	{
		Label->setString(Wanted);
	}
	Label->setScale(1.0f);
	const float Width = Label->getContentSize().width;
	if ( !ShrinksText(InOutPart) || InLimit <= 0.0f || Width <= InLimit )
	{
		return;
	}
	const float Factor = InLimit / Width;
	if ( Child.MinScale <= 0.0f || Factor >= Child.MinScale )
	{
		Label->setScale(Factor);
		return;
	}
	Label->setScale(Child.MinScale);
	size_t Keep = CountCharacters(Wanted);
	while ( Keep > 1 && Label->getContentSize().width * Child.MinScale > InLimit )
	{
		Keep = std::min(Keep - 1, static_cast<size_t>(static_cast<float>(Keep) * InLimit / (Label->getContentSize().width * Child.MinScale)));
		Keep = std::max<size_t>(Keep, 1);
		Label->setString(CutWithEllipsis(Wanted, Keep));
	}
}

void LayoutWidget::DrawRect(Part& InOutPart)
{
	const cocos2d::Size Half = InOutPart.Size / 2;
	InOutPart.Drawing->clear();
	if ( InOutPart.Size.width <= 0.0f || InOutPart.Size.height <= 0.0f )
	{
		return;
	}
	const std::optional<model::Color> Edge = Resolve(BorderOf(InOutPart));
	const float Border = Edge ? InOutPart.Def.BorderWidth : 0.0f;
	if ( Edge && Border > 0.0f )
	{
		FillRounded(InOutPart.Drawing, Half, InOutPart.Def.Radius, cocos2d::Color4F(ToCocos(*Edge)));
	}
	if ( const std::optional<model::Color> Fill = Resolve(FillOf(InOutPart)) )
	{
		FillRounded(InOutPart.Drawing, Half - cocos2d::Size(Border, Border), std::max(InOutPart.Def.Radius - Border, 0.0f), cocos2d::Color4F(ToCocos(*Fill)));
	}
}

void LayoutWidget::DrawRing(Part& InOutPart)
{
	InOutPart.Drawing->clear();
	const std::optional<model::Color> Fill = Resolve(FillOf(InOutPart));
	const float Radius = std::min(InOutPart.Size.width, InOutPart.Size.height) / 2;
	if ( !Fill || Radius <= 0.0f )
	{
		return;
	}
	const cocos2d::Color4F Color(ToCocos(*Fill));
	const float Thickness = InOutPart.Def.BorderWidth;
	constexpr int Segments = 24;
	constexpr float StartAngle = 0.5f;
	constexpr float EndAngle = 5.5f;
	const auto Point = [](const float InAngle, const float InRadius)
	{
		return cocos2d::Vec2(std::cos(InAngle), std::sin(InAngle)) * InRadius;
	};
	for ( int Step = 0; Step < Segments; ++Step )
	{
		const float From = StartAngle + (EndAngle - StartAngle) * Step / Segments;
		const float To = StartAngle + (EndAngle - StartAngle) * (Step + 1) / Segments;
		const std::array<cocos2d::Vec2, 4> Quad = { Point(From, Radius + Thickness / 2), Point(To, Radius + Thickness / 2), Point(To, Radius - Thickness / 2), Point(From, Radius - Thickness / 2) };
		InOutPart.Drawing->drawSolidPoly(Quad.data(), static_cast<unsigned int>(Quad.size()), Color);
	}
	const cocos2d::Vec2 Forward(-std::sin(EndAngle), std::cos(EndAngle));
	const float Head = Thickness * 2.2f;
	const std::array<cocos2d::Vec2, 3> Arrow = { Point(EndAngle, Radius) + Forward * Head, Point(EndAngle, Radius + Head * 0.8f), Point(EndAngle, Radius - Head * 0.8f) };
	InOutPart.Drawing->drawSolidPoly(Arrow.data(), static_cast<unsigned int>(Arrow.size()), Color);
}

void LayoutWidget::DrawBar(Part& InOutPart)
{
	const cocos2d::Size Half = InOutPart.Size / 2;
	InOutPart.Drawing->clear();
	if ( InOutPart.Size.width <= 0.0f || InOutPart.Size.height <= 0.0f )
	{
		return;
	}
	if ( const std::optional<model::Color> Track = Resolve(InOutPart.Def.BackColor) )
	{
		InOutPart.Drawing->drawSolidRect(cocos2d::Vec2(-Half.width, -Half.height), cocos2d::Vec2(Half.width, Half.height), cocos2d::Color4F(ToCocos(*Track)));
	}
	const auto DrawShare = [&](const float InShare, const std::optional<model::Color>& InColor)
	{
		if ( InColor && InShare > 0.0f )
		{
			InOutPart.Drawing->drawSolidRect(cocos2d::Vec2(-Half.width, -Half.height), cocos2d::Vec2(-Half.width + InOutPart.Size.width * InShare, Half.height), cocos2d::Color4F(ToCocos(*InColor)));
		}
	};
	if ( InOutPart.Meter.GetTrail() > InOutPart.Meter.GetShown() )
	{
		DrawShare(InOutPart.Meter.GetTrail(), Resolve(InOutPart.Def.TrailColor));
	}
	DrawShare(InOutPart.Meter.GetShown(), Resolve(InOutPart.Def.FillColor));
}

void LayoutWidget::Resize(Part& InOutPart, const cocos2d::Size& InSize)
{
	InOutPart.Size = InSize;
	if ( InOutPart.Drawing )
	{
		switch ( InOutPart.Def.Part )
		{
			case viewmodel::WidgetPart::Ring:
				DrawRing(InOutPart);
				break;
			case viewmodel::WidgetPart::Bar:
				DrawBar(InOutPart);
				break;
			default:
				DrawRect(InOutPart);
				break;
		}
	}
	else if ( InOutPart.Sub && (InOutPart.Def.bFill || InOutPart.Def.bStretch || InOutPart.Def.Grow > 0.0f) )
	{
		InOutPart.Sub->FixedOverride = InSize;
		InOutPart.Sub->Arrange();
	}
}

cocos2d::Size LayoutWidget::Arrange()
{
	const float Scale = GetScale();
	model::LayoutRules Rules = Def.Rules;
	Rules.Gap = (GapOverride >= 0.0f ? GapOverride : Rules.Gap) * Scale;
	Rules.Padding = Rules.Padding * Scale;
	Rules.FixedSize = FixedOverride.width > 0.0f || FixedOverride.height > 0.0f ? ToModel(FixedOverride) : Rules.FixedSize * Scale;
	Rules.MinSize = Rules.MinSize * Scale;

	const bool bRow = Rules.Kind == model::LayoutKind::Row;
	const float InnerWidth = Rules.FixedSize.X > 0.0f ? Rules.FixedSize.X - Rules.Padding.X * 2.0f : 0.0f;
	std::vector<model::LayoutChild> Items(Parts.size());
	for ( size_t Index = 0; Index < Parts.size(); ++Index )
	{
		Part& Current = Parts[Index];
		model::LayoutChild& Item = Items[Index];
		Item.Anchor = Current.Def.Anchor;
		Item.Offset = Current.Def.Offset * Scale;
		Item.Pivot = Current.Def.Pivot;
		Item.bHasPivot = Current.Def.bHasPivot;
		Item.bFill = Current.Def.bFill;
		Item.Grow = Current.Def.Grow;
		Item.bStretch = Current.Def.bStretch;
		Item.bHidden = !Current.Node->isVisible();
		if ( !Item.bFill )
		{
			Item.Size = ToModel(Measure(Current, bRow ? 0.0f : InnerWidth));
		}
	}
	if ( bRow && InnerWidth > 0.0f )
	{
		// A text that shrinks takes what its siblings in the row leave.
		float Others = Rules.Gap * static_cast<float>(Parts.size() > 1 ? Parts.size() - 1 : 0);
		for ( size_t Index = 0; Index < Parts.size(); ++Index )
		{
			const bool bShrinks = ShrinksText(Parts[Index]);
			Others += bShrinks || Parts[Index].Def.bFill ? 0.0f : Items[Index].Size.X;
		}
		for ( size_t Index = 0; Index < Parts.size(); ++Index )
		{
			if ( ShrinksText(Parts[Index]) )
			{
				Items[Index].Size = ToModel(Measure(Parts[Index], std::max(InnerWidth - Others, 1.0f)));
			}
		}
	}

	const model::Vec2f Size = model::widget_layout::Arrange(Rules, Items);
	for ( size_t Index = 0; Index < Parts.size(); ++Index )
	{
		Part& Current = Parts[Index];
		Current.LayoutPosition = cocos2d::Vec2(Items[Index].Position.X, Items[Index].Position.Y);
		Current.Node->setPosition(Current.LayoutPosition);
		Resize(Current, ToSize(Items[Index].Size));
		if ( Current.Player )
		{
			if ( !Current.TextLabel && !Current.Picture )
			{
				Current.Node->setScale(Current.Def.Scale);
			}
			Current.BaseScaleX = Current.Node->getScaleX();
			Current.BaseScaleY = Current.Node->getScaleY();
			ApplyAnimation(Current);
		}
	}
	const cocos2d::Size Result = ToSize(Size);
	setContentSize(Result);
	return Result;
}

const viewmodel::WidgetDef& LayoutWidget::GetDef() const
{
	return Def;
}

bool LayoutWidget::Activate() const
{
	if ( Def.OnClick.empty() || !Handler )
	{
		return false;
	}
	Handler(Def.OnClick);
	return true;
}

void LayoutWidget::SetCommandHandler(const CommandHandler& InHandler)
{
	Handler = InHandler;
}

} // namespace game::view::cocos

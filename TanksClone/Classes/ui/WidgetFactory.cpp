#include "ui/WidgetFactory.h"

#include "core/ResourceIds.h"

namespace game::view::cocos
{

LayoutWidget* WidgetFactory::Create(const std::string_view InId)
{
	return LayoutWidget::Create(InId);
}

LayoutWidget* WidgetFactory::CreateKeycap(const std::string& InKeyName)
{
	LayoutWidget* const Keycap = LayoutWidget::Create(ids::widgets::Keycap);
	if ( Keycap )
	{
		Keycap->SetText("text", InKeyName);
	}
	return Keycap;
}

LayoutWidget* WidgetFactory::CreateIcon(const std::string& InFrame)
{
	LayoutWidget* const Icon = LayoutWidget::Create(ids::widgets::Icon);
	if ( Icon )
	{
		Icon->SetIconFrame("image", InFrame);
	}
	return Icon;
}

} // namespace game::view::cocos

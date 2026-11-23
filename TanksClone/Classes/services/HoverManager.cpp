#include "services/HoverManager.h"

#include <algorithm>

#include "cocos2d.h"

#include "ui/LayoutWidget.h"

namespace tanks
{

HoverManager::HoverManager()
{
	Listener = cocos2d::EventListenerMouse::create();
	Listener->onMouseMove = [this](cocos2d::Event* const InEvent)
	{
		const auto Mouse = static_cast<cocos2d::EventMouse*>(InEvent);
		OnMouseMove(Mouse->getCursorX(), Mouse->getCursorY());
	};
	Listener->retain();
	cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(Listener, 1);
}

HoverManager::~HoverManager()
{
	if ( Listener )
	{
		cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(Listener);
		Listener->release();
	}
}

void HoverManager::Register(view::cocos::LayoutWidget* const InWidget)
{
	const auto Found = std::find_if(Entries.begin(), Entries.end(), [InWidget](const Entry& InEntry) { return InEntry.Widget == InWidget; });
	if ( Found != Entries.end() )
	{
		return;
	}
	Entries.push_back({ InWidget, Book.Add(InWidget->GetWorldBounds(), InWidget->GetHoverLayer(), true) });
}

void HoverManager::Unregister(view::cocos::LayoutWidget* const InWidget)
{
	const auto Found = std::find_if(Entries.begin(), Entries.end(), [InWidget](const Entry& InEntry) { return InEntry.Widget == InWidget; });
	if ( Found == Entries.end() )
	{
		return;
	}
	if ( Found->Id == HoveredId )
	{
		HoveredId = model::HoverBook::NoWidget;
	}
	Book.Remove(Found->Id);
	Entries.erase(Found);
}

void HoverManager::SetModalLayer(const int InLayer)
{
	Book.SetModalLayer(InLayer);
	Refresh();
}

void HoverManager::PushModal(const cocos2d::Node* const InRoot)
{
	ModalRoots.push_back(InRoot);
	Refresh();
}

void HoverManager::PopModal(const cocos2d::Node* const InRoot)
{
	ModalRoots.erase(std::remove(ModalRoots.begin(), ModalRoots.end(), InRoot), ModalRoots.end());
	Refresh();
}

bool HoverManager::IsReachable(const cocos2d::Node* const InWidget) const
{
	if ( ModalRoots.empty() )
	{
		return true;
	}
	for ( const cocos2d::Node* Current = InWidget; Current; Current = Current->getParent() )
	{
		if ( Current == ModalRoots.back() )
		{
			return true;
		}
	}
	return false;
}

view::cocos::LayoutWidget* HoverManager::WidgetOf(const int InId) const
{
	const auto Found = std::find_if(Entries.begin(), Entries.end(), [InId](const Entry& InEntry) { return InEntry.Id == InId; });
	return Found == Entries.end() ? nullptr : Found->Widget;
}

void HoverManager::OnMouseMove(const float InX, const float InY)
{
	PointerX = InX;
	PointerY = InY;
	Refresh();
}

void HoverManager::Refresh()
{
	for ( const Entry& Current : Entries )
	{
		Book.Update(Current.Id, Current.Widget->GetWorldBounds(), Current.Widget->GetHoverLayer(), Current.Widget->isVisible() && IsReachable(Current.Widget));
	}
	Book.MovePointer(model::Vec2f(PointerX, PointerY));
	const int Now = Book.GetHovered();
	if ( Now == HoveredId )
	{
		return;
	}
	if ( view::cocos::LayoutWidget* const Old = WidgetOf(HoveredId) )
	{
		Old->SetHovered(false);
	}
	HoveredId = Now;
	if ( view::cocos::LayoutWidget* const Next = WidgetOf(HoveredId) )
	{
		Next->SetHovered(true);
	}
}

} // namespace tanks

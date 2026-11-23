#pragma once

#include <vector>

#include "model/HoverBook.h"

namespace cocos2d
{
class EventListenerMouse;
}

namespace tanks
{

namespace view::cocos
{
class LayoutWidget;
}

/**
 * Tells widgets that the mouse is over them. Clickable widgets register while they are on screen; on every mouse move
 * the model::HoverBook picks the one under the pointer (highest layer wins) and only that widget is hovered.
 * The mouse is read straight from the engine until IInputSource carries a pointer.
 */
class HoverManager
{
public:
	HoverManager();
	~HoverManager();
	HoverManager(const HoverManager&) = delete;
	HoverManager& operator=(const HoverManager&) = delete;

	void Register(view::cocos::LayoutWidget* InWidget);
	void Unregister(view::cocos::LayoutWidget* InWidget);

	// Entries below this layer cannot be hovered (an open dialog sets its own).
	void SetModalLayer(int InLayer);

private:
	struct Entry
	{
		view::cocos::LayoutWidget* Widget = nullptr;
		int Id = model::HoverBook::NoWidget;
	};

	void OnMouseMove(float InX, float InY);
	void Refresh();
	view::cocos::LayoutWidget* WidgetOf(int InId) const;

	model::HoverBook Book;
	std::vector<Entry> Entries;
	cocos2d::EventListenerMouse* Listener = nullptr;
	float PointerX = -1.0e9f;
	float PointerY = -1.0e9f;
	int HoveredId = model::HoverBook::NoWidget;
};

} // namespace tanks

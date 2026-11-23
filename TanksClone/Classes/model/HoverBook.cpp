#include "model/HoverBook.h"

#include <algorithm>

namespace tanks::model
{

int HoverBook::Add(const RectF& InBounds, const int InLayer, const bool bInActive)
{
	Entry NewEntry;
	NewEntry.Id = NextId++;
	NewEntry.Bounds = InBounds;
	NewEntry.Layer = InLayer;
	NewEntry.bActive = bInActive;
	Entries.push_back(NewEntry);
	return NewEntry.Id;
}

void HoverBook::Update(const int InId, const RectF& InBounds, const int InLayer, const bool bInActive)
{
	if ( Entry* const Found = Find(InId) )
	{
		Found->Bounds = InBounds;
		Found->Layer = InLayer;
		Found->bActive = bInActive;
	}
}

void HoverBook::Remove(const int InId)
{
	Entries.erase(std::remove_if(Entries.begin(), Entries.end(), [InId](const Entry& InEntry) { return InEntry.Id == InId; }), Entries.end());
	if ( Hovered == InId )
	{
		Hovered = NoWidget;
	}
}

void HoverBook::Clear()
{
	Entries.clear();
	Hovered = NoWidget;
}

void HoverBook::SetModalLayer(const int InLayer)
{
	ModalLayer = InLayer;
}

int HoverBook::GetModalLayer() const
{
	return ModalLayer;
}

int HoverBook::Pick(const Vec2f& InPoint) const
{
	int Best = NoWidget;
	int BestLayer = 0;
	for ( const Entry& Current : Entries )
	{
		if ( !Current.bActive || Current.Layer < ModalLayer || !Current.Bounds.Contains(InPoint) )
		{
			continue;
		}
		if ( Best == NoWidget || Current.Layer >= BestLayer )
		{
			Best = Current.Id;
			BestLayer = Current.Layer;
		}
	}
	return Best;
}

bool HoverBook::MovePointer(const Vec2f& InPoint)
{
	const int Now = Pick(InPoint);
	const bool bChanged = Now != Hovered;
	Hovered = Now;
	return bChanged;
}

int HoverBook::GetHovered() const
{
	return Hovered;
}

HoverBook::Entry* HoverBook::Find(const int InId)
{
	const auto Found = std::find_if(Entries.begin(), Entries.end(), [InId](const Entry& InEntry) { return InEntry.Id == InId; });
	return Found == Entries.end() ? nullptr : &*Found;
}

} // namespace tanks::model

#pragma once

#include <vector>

#include "model/MathTypes.h"

namespace game::model
{
/**
 * Decides which one widget the pointer hovers. Widgets register a rectangle and a layer number; the highest layer
 * under the pointer wins, and among equal layers the one added last. A modal layer hides everything below it.
 * Engine-free: the view feeds it rectangles and the pointer position.
 */
class HoverBook
{
public:
	static constexpr int NoWidget = -1;

	int Add(const RectF& InBounds, int InLayer, bool bInActive = true);
	void Update(int InId, const RectF& InBounds, int InLayer, bool bInActive);
	void Remove(int InId);
	void Clear();

	void SetModalLayer(int InLayer);
	int GetModalLayer() const;

	int Pick(const Vec2f& InPoint) const;

	bool MovePointer(const Vec2f& InPoint);
	int GetHovered() const;

private:
	struct Entry
	{
		int Id = NoWidget;
		RectF Bounds;
		int Layer = 0;
		bool bActive = true;
	};

	Entry* Find(int InId);

	std::vector<Entry> Entries;
	int NextId = 0;
	int ModalLayer = 0;
	int Hovered = NoWidget;
};
} // namespace game::model

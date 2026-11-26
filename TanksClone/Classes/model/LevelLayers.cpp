#include "model/LevelLayers.h"

#include <algorithm>

namespace game::model
{

namespace
{
	bool ReadCell(const IConfigNode& InNode, int& OutColumn, int& OutRow)
	{
		Vec2f Cell;
		if ( !InNode.TryRead("cell", Cell) )
		{
			return false;
		}
		OutColumn = static_cast<int>(Cell.X);
		OutRow = static_cast<int>(Cell.Y);
		return true;
	}

	JsonValue CellToJson(const int InColumn, const int InRow)
	{
		return JsonValue(Vec2f{ static_cast<float>(InColumn), static_cast<float>(InRow) });
	}
}

LevelLayers LevelLayers::FromConfig(const IConfigNode& InNode)
{
	LevelLayers Result;
	if ( const IConfigNode* const PointList = InNode.FindChild("points") )
	{
		for ( const std::string& Key : PointList->GetKeys() )
		{
			Vec2f Cell;
			if ( PointList->TryRead(Key, Cell) )
			{
				Result.SetPoint(Key, static_cast<int>(Cell.X), static_cast<int>(Cell.Y));
			}
		}
	}
	if ( const IConfigNode* const ObjectList = InNode.FindChild("objects") )
	{
		for ( std::size_t Index = 0; Index < ObjectList->GetItemCount(); ++Index )
		{
			const IConfigNode& Item = *ObjectList->GetItem(Index);
			LevelObject Object;
			if ( Item.TryRead("config", Object.Config) && ReadCell(Item, Object.Column, Object.Row) )
			{
				Item.TryRead("name", Object.Name);
				Result.Objects.PushBack(Object);
			}
		}
	}
	return Result;
}

JsonValue LevelLayers::ObjectsToJson() const
{
	JsonValue::Array Items;
	for ( const LevelObject& Object : Objects )
	{
		JsonValue::Object Members;
		Members.InsertOrAssign("config", JsonValue(Object.Config));
		Members.InsertOrAssign("cell", CellToJson(Object.Column, Object.Row));
		if ( !Object.Name.empty() )
		{
			Members.InsertOrAssign("name", JsonValue(Object.Name));
		}
		Items.PushBack(JsonValue(std::move(Members)));
	}
	return JsonValue(std::move(Items));
}

JsonValue LevelLayers::PointsToJson() const
{
	JsonValue::Object Members;
	for ( const LevelPoint& Point : Points )
	{
		Members.InsertOrAssign(Point.Name, CellToJson(Point.Column, Point.Row));
	}
	return JsonValue(std::move(Members));
}

const LevelPoint* LevelLayers::FindPoint(const std::string_view InName) const
{
	for ( const LevelPoint& Point : Points )
	{
		if ( Point.Name == InName )
		{
			return &Point;
		}
	}
	return nullptr;
}

bool LevelLayers::SetPoint(const std::string_view InName, const int InColumn, const int InRow)
{
	if ( InName.empty() )
	{
		return false;
	}
	for ( LevelPoint& Point : Points )
	{
		if ( Point.Name == InName )
		{
			Point.Column = InColumn;
			Point.Row = InRow;
			return true;
		}
	}
	Points.PushBack(LevelPoint{ std::string(InName), InColumn, InRow });
	return true;
}

bool LevelLayers::RemovePoint(const std::string_view InName)
{
	for ( std::size_t Index = 0; Index < Points.Size(); ++Index )
	{
		if ( Points[Index].Name == InName )
		{
			Points.RemoveAt(Index);
			return true;
		}
	}
	return false;
}

std::string LevelLayers::MakePointName() const
{
	for ( int Number = 1;; ++Number )
	{
		const std::string Name = "p" + std::to_string(Number);
		if ( !FindPoint(Name) )
		{
			return Name;
		}
	}
}

bool LevelLayers::RenamePoint(const std::string_view InOldName, const std::string_view InNewName)
{
	if ( InNewName.empty() || (InOldName != InNewName && FindPoint(InNewName)) )
	{
		return false;
	}
	for ( LevelPoint& Point : Points )
	{
		if ( Point.Name == InOldName )
		{
			Point.Name = std::string(InNewName);
			return true;
		}
	}
	return false;
}

const LevelPoint* LevelLayers::FindPointAt(const int InColumn, const int InRow) const
{
	for ( const LevelPoint& Point : Points )
	{
		if ( Point.Column == InColumn && Point.Row == InRow )
		{
			return &Point;
		}
	}
	return nullptr;
}

const LevelObject* LevelLayers::FindObjectAt(const int InColumn, const int InRow) const
{
	for ( std::size_t Index = Objects.Size(); Index > 0; --Index )
	{
		const LevelObject& Object = Objects[Index - 1];
		if ( Object.Column == InColumn && Object.Row == InRow )
		{
			return &Object;
		}
	}
	return nullptr;
}

void LevelLayers::AddObject(const LevelObject& InObject)
{
	Objects.PushBack(InObject);
}

bool LevelLayers::RemoveObjectAt(const int InColumn, const int InRow)
{
	for ( std::size_t Index = Objects.Size(); Index > 0; --Index )
	{
		const LevelObject& Object = Objects[Index - 1];
		if ( Object.Column == InColumn && Object.Row == InRow )
		{
			Objects.RemoveAt(Index - 1);
			return true;
		}
	}
	return false;
}

TArray<std::string> LevelLayers::FindMissingPoints(const TArray<std::string>& InLinks) const
{
	TArray<std::string> Missing;
	for ( const std::string& Link : InLinks )
	{
		if ( !FindPoint(Link) && std::find(Missing.begin(), Missing.end(), Link) == Missing.end() )
		{
			Missing.PushBack(Link);
		}
	}
	return Missing;
}

void LevelLayers::Crop(const int InColumns, const int InRows, const int InShiftColumns, const int InShiftRows)
{
	const auto IsOutside = [InColumns, InRows](const int InColumn, const int InRow)
	{
		return InColumn < 0 || InRow < 0 || InColumn >= InColumns || InRow >= InRows;
	};
	TArray<LevelPoint> KeptPoints;
	for ( LevelPoint Point : Points )
	{
		Point.Column += InShiftColumns;
		Point.Row += InShiftRows;
		if ( !IsOutside(Point.Column, Point.Row) )
		{
			KeptPoints.PushBack(Point);
		}
	}
	Points = std::move(KeptPoints);
	TArray<LevelObject> KeptObjects;
	for ( LevelObject Object : Objects )
	{
		Object.Column += InShiftColumns;
		Object.Row += InShiftRows;
		if ( !IsOutside(Object.Column, Object.Row) )
		{
			KeptObjects.PushBack(Object);
		}
	}
	Objects = std::move(KeptObjects);
}

} // namespace game::model

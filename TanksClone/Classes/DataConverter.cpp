#include "DataConverter.h"


USING_NS_CC;

Vec2 DataConverter::ValueToVec2(const Value& aValue)
{
	Vec2 result;
	if ( aValue.getType() == Value::Type::VECTOR )
	{
		const ValueVector& valueVec = aValue.asValueVector();
		if ( valueVec.size() > 1 )
		{
			result.x = valueVec.at(0).asFloat();
			result.y = valueVec.at(1).asFloat();
		}
	}

	return result;
}

Color3B DataConverter::ValueToColor3B(const Value& aValue)
{
	Color3B result;
	if ( aValue.getType() == Value::Type::VECTOR )
	{
		const ValueVector& valueVec = aValue.asValueVector();
		if ( valueVec.size() > 2 )
		{
			result.r = valueVec.at(0).asFloat();
			result.g = valueVec.at(1).asFloat();
			result.b = valueVec.at(2).asFloat();
		}
	}

	return result;
}

std::map<std::string, std::string> DataConverter::ValueToStringMap(const Value& aValue)
{
	return std::map<std::string, std::string>();
}

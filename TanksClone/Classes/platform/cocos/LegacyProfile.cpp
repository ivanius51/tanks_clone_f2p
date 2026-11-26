#include "platform/cocos/LegacyProfile.h"

#include "cocos2d.h"

#include "model/JsonValue.h"

namespace game::view::cocos
{

namespace
{
	constexpr const char* LegacyFile = "profile.plist";
	constexpr const char* CurrentFile = "profile.json";

	model::JsonValue ToJson(const cocos2d::Value& InValue)
	{
		switch ( InValue.getType() )
		{
			case cocos2d::Value::Type::BOOLEAN: return model::JsonValue(InValue.asBool());
			case cocos2d::Value::Type::INTEGER: return model::JsonValue(InValue.asInt());
			case cocos2d::Value::Type::FLOAT:
			case cocos2d::Value::Type::DOUBLE: return model::JsonValue(InValue.asDouble());
			case cocos2d::Value::Type::STRING: return model::JsonValue(InValue.asString());
			case cocos2d::Value::Type::MAP:
			{
				model::JsonValue::Object Members;
				for ( const auto& Member : InValue.asValueMap() )
				{
					Members.InsertOrAssign(Member.first, ToJson(Member.second));
				}
				return model::JsonValue(std::move(Members));
			}
			default: return model::JsonValue();
		}
	}
}

bool MigrateLegacyProfile(ports::IStorage& InStorage)
{
	if ( InStorage.Exists(CurrentFile) )
	{
		return false;
	}
	const std::optional<std::string> Text = InStorage.ReadText(LegacyFile);
	if ( !Text )
	{
		return false;
	}
	const cocos2d::ValueMap Values = cocos2d::FileUtils::getInstance()->getValueMapFromData(Text->c_str(), static_cast<int>(Text->size()));
	if ( Values.empty() )
	{
		return false;
	}
	model::JsonValue::Object Root;
	for ( const auto& Member : Values )
	{
		Root.InsertOrAssign(Member.first, ToJson(cocos2d::Value(Member.second)));
	}
	return InStorage.WriteText(CurrentFile, model::JsonValue(std::move(Root)).ToString());
}

} // namespace game::view::cocos

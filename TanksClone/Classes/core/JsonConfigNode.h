#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"
#include "templates/TUniquePtr.h"

namespace game
{
	class JsonConfigNode;
	using TJsonConfigNodeHandle = THandle<JsonConfigNode>;
/** IConfigNode over parsed JSON. The only place loaders' data meets the JSON format. */
class JsonConfigNode final : public model::IConfigNode
{
public:
	JsonConfigNode(const model::JsonValue& InValue, std::string_view InSource);

	static TJsonConfigNodeHandle Parse(std::string_view InText, std::string_view InSource, bool* bOutOk = nullptr);

	const IConfigNode* FindChild(std::string_view InKey) const override;
	std::vector<std::string> GetKeys() const override;
	std::size_t GetItemCount() const override;
	const IConfigNode* GetItem(std::size_t InIndex) const override;

	std::optional<bool> ReadBool() const override;
	std::optional<double> ReadNumber() const override;
	std::optional<std::string> ReadString() const override;

protected:
	void ReportMissing(std::string_view InKey) const override;

private:
	model::JsonValue Value;
	std::string Source;
	std::vector<std::pair<std::string, TUniquePtr<JsonConfigNode>>> Members;
	std::vector<TUniquePtr<JsonConfigNode>> Items;
};
} // namespace game

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"

namespace game
{
/** IConfigNode over parsed JSON. The only place loaders' data meets the JSON format. */
class JsonConfigNode final : public model::IConfigNode
{
public:
	JsonConfigNode(const model::JsonValue& InValue, std::string_view InSource);

	static THandle<JsonConfigNode> Parse(std::string_view InText, std::string_view InSource, bool* bOutOk = nullptr);

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
	std::vector<std::pair<std::string, std::unique_ptr<JsonConfigNode>>> Members;
	std::vector<std::unique_ptr<JsonConfigNode>> Items;
};
} // namespace game

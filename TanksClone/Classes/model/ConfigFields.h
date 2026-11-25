#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"

namespace game::model
{
/**
 * Reads optional config fields into values that already hold their defaults, and records the path of every field
 * that is missing or has the wrong type ("hud.top.height"), so the loader can log them.
 */
class ConfigFields
{
public:
	ConfigFields(const IConfigNode* InNode, std::vector<std::string>& OutMissing, std::string InPath = "")
		: Node(InNode), Missing(OutMissing), Path(std::move(InPath))
	{
	}

	template <typename TValue>
	void Read(const std::string_view InKey, TValue& InOutValue) const
	{
		if ( !Node || !Node->TryRead(InKey, InOutValue) )
		{
			Missing.push_back(Path + std::string(InKey));
		}
	}

	ConfigFields Child(const std::string_view InKey) const
	{
		return ConfigFields(Node ? Node->FindChild(InKey) : nullptr, Missing, Path + std::string(InKey) + ".");
	}

	const IConfigNode* GetNode() const
	{
		return Node;
	}

private:
	const IConfigNode* Node;
	std::vector<std::string>& Missing;
	std::string Path;
};
} // namespace game::model

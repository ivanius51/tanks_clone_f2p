#pragma once

#include <string_view>

namespace tanks::model
{
/**
 * Names one resource ("sfx.player_shot"). It only views the name, so the name must outlive the id:
 * ids are constants built from string literals (see core/ResourceIds.h). Paths are never kept here;
 * the resource table maps an id to its file.
 */
class ResourceId
{
public:
	constexpr ResourceId() = default;
	constexpr explicit ResourceId(const std::string_view InName) : Name(InName)
	{
	}

	constexpr std::string_view GetName() const
	{
		return Name;
	}

	constexpr bool IsValid() const
	{
		return !Name.empty();
	}

	constexpr bool operator==(const ResourceId& InOther) const
	{
		return Name == InOther.Name;
	}

	constexpr bool operator!=(const ResourceId& InOther) const
	{
		return Name != InOther.Name;
	}

private:
	std::string_view Name;
};
} // namespace tanks::model

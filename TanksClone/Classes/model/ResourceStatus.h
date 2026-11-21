#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace tanks::model
{

enum class ResourceState
{
	Unchecked,
	Ok,
	Missing,
};

/**
 * What is known about each resource the game asked for, by kind and name ("atlas", "atlas.common";
 * "frame", "tank_1"): not tried yet, loaded fine, or missing. A missing resource is reported once and not
 * tried again until Forget() or ForgetMissing() (e.g. after the texture set changes), so a freed resource
 * that is loaded again later does not hit the disk or the log on every request.
 */
class ResourceStatusBook
{
public:
	ResourceState Get(std::string_view InKind, std::string_view InName) const;
	bool IsMissing(std::string_view InKind, std::string_view InName) const;
	void MarkOk(std::string_view InKind, std::string_view InName);
	// True the first time this resource is marked missing: the caller logs it then.
	bool MarkMissing(std::string_view InKind, std::string_view InName);
	void Forget(std::string_view InKind, std::string_view InName);
	void ForgetMissing();

	std::size_t Count(ResourceState InState) const;
	// "kind name" of every missing resource, sorted.
	std::vector<std::string> GetMissing() const;

private:
	static std::string KeyOf(std::string_view InKind, std::string_view InName);

	std::map<std::string, ResourceState> States;
};

} // namespace tanks::model

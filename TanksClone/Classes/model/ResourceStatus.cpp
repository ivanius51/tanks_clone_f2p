#include "model/ResourceStatus.h"

#include <iterator>

namespace game::model
{

std::string ResourceStatusBook::KeyOf(const std::string_view InKind, const std::string_view InName)
{
	std::string Key(InKind);
	Key += ' ';
	Key += InName;
	return Key;
}

ResourceState ResourceStatusBook::Get(const std::string_view InKind, const std::string_view InName) const
{
	const auto Found = States.find(KeyOf(InKind, InName));
	return Found != States.end() ? Found->second : ResourceState::Unchecked;
}

bool ResourceStatusBook::IsMissing(const std::string_view InKind, const std::string_view InName) const
{
	return Get(InKind, InName) == ResourceState::Missing;
}

void ResourceStatusBook::MarkOk(const std::string_view InKind, const std::string_view InName)
{
	States[KeyOf(InKind, InName)] = ResourceState::Ok;
}

bool ResourceStatusBook::MarkMissing(const std::string_view InKind, const std::string_view InName)
{
	ResourceState& State = States[KeyOf(InKind, InName)];
	const bool bFirstTime = State != ResourceState::Missing;
	State = ResourceState::Missing;
	return bFirstTime;
}

void ResourceStatusBook::Forget(const std::string_view InKind, const std::string_view InName)
{
	States.erase(KeyOf(InKind, InName));
}

void ResourceStatusBook::ForgetMissing()
{
	for ( auto It = States.begin(); It != States.end(); )
	{
		It = It->second == ResourceState::Missing ? States.erase(It) : std::next(It);
	}
}

std::size_t ResourceStatusBook::Count(const ResourceState InState) const
{
	if ( InState == ResourceState::Unchecked )
	{
		return 0;
	}
	std::size_t Result = 0;
	for ( const auto& Entry : States )
	{
		Result += Entry.second == InState ? 1 : 0;
	}
	return Result;
}

std::vector<std::string> ResourceStatusBook::GetMissing() const
{
	std::vector<std::string> Result;
	for ( const auto& Entry : States )
	{
		if ( Entry.second == ResourceState::Missing )
		{
			Result.push_back(Entry.first);
		}
	}
	return Result;
}

} // namespace game::model

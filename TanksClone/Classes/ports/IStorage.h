#pragma once

#include <optional>
#include <string>

namespace game::ports
{
/**
 * Port: keeps small named text files that survive between runs (the profile, a custom level).
 * A name is a plain file name such as "profile.json"; where it lives (a writable folder, a cloud save,
 * browser storage) is the adapter's business.
 */
class IStorage
{
public:
	virtual ~IStorage() = default;

	virtual bool Exists(const std::string& InName) const = 0;
	virtual std::optional<std::string> ReadText(const std::string& InName) const = 0;
	virtual bool WriteText(const std::string& InName, const std::string& InText) = 0;
};
} // namespace game::ports

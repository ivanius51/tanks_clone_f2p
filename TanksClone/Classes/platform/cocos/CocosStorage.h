#pragma once

#include "ports/IStorage.h"

namespace tanks::view::cocos
{

/** IStorage over the engine's writable folder. */
class CocosStorage final : public ports::IStorage
{
public:
	bool Exists(const std::string& InName) const override;
	std::optional<std::string> ReadText(const std::string& InName) const override;
	bool WriteText(const std::string& InName, const std::string& InText) override;

private:
	static std::string PathOf(const std::string& InName);
};

} // namespace tanks::view::cocos

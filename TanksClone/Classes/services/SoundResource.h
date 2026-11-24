#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "model/RefCounted.h"
#include "ports/IAudioOutput.h"

namespace tanks
{
class SoundResource;

/** The sounds that are loaded now, by id name, and the requests still loading. Outlives the sounds that point to it. */
struct SoundRegistry
{
	using Callback = std::function<void(model::Handle<SoundResource>)>;

	ports::IAudioOutput* Audio = nullptr;
	std::map<std::string, SoundResource*> Loaded;
	std::map<std::string, std::vector<Callback>> Pending;
};

/** One sound effect loaded into the audio engine's cache. It stays cached while a handle to it exists. */
class SoundResource final : public model::RefCounted
{
public:
	static model::Handle<SoundResource> Create(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry);
	static void CreateAsync(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry, const SoundRegistry::Callback& InDone);

	const std::string& GetName() const;

private:
	SoundResource(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry);
	~SoundResource() override;

	std::string Name;
	std::string Path;
	std::shared_ptr<SoundRegistry> Registry;
};
} // namespace tanks

#pragma once

#include <map>
#include <memory>
#include <string>

#include "model/RefCounted.h"

namespace tanks
{

class SoundResource;

/** The sounds that are loaded now, by id name. Outlives the sounds that point to it. */
struct SoundRegistry
{
	std::map<std::string, SoundResource*> Loaded;
};

/** One sound effect loaded into the audio engine. It stays loaded while a handle to it exists. */
class SoundResource final : public model::RefCounted
{
public:
	static model::Handle<SoundResource> Create(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry);

	const std::string& GetName() const;

private:
	SoundResource(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry);
	~SoundResource() override;

	std::string Name;
	std::string Path;
	std::shared_ptr<SoundRegistry> Registry;
};

} // namespace tanks

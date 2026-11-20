#include "services/SoundResource.h"

#include "audio/include/SimpleAudioEngine.h"

namespace tanks
{

model::Handle<SoundResource> SoundResource::Create(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry)
{
	return model::Handle<SoundResource>(new SoundResource(InName, InPath, InRegistry));
}

SoundResource::SoundResource(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry)
	: Name(InName), Path(InPath), Registry(InRegistry)
{
	CocosDenshion::SimpleAudioEngine::getInstance()->preloadEffect(Path.c_str());
	Registry->Loaded[Name] = this;
}

SoundResource::~SoundResource()
{
	Registry->Loaded.erase(Name);
	CocosDenshion::SimpleAudioEngine::getInstance()->unloadEffect(Path.c_str());
}

const std::string& SoundResource::GetName() const
{
	return Name;
}

} // namespace tanks

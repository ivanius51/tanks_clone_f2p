#include "services/SoundResource.h"

#include "audio/include/AudioEngine.h"

namespace tanks
{

model::Handle<SoundResource> SoundResource::Create(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry)
{
	model::Handle<SoundResource> Result(new SoundResource(InName, InPath, InRegistry));
	cocos2d::experimental::AudioEngine::preload(InPath, [](bool) {});
	return Result;
}

void SoundResource::CreateAsync(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry, const SoundRegistry::Callback& InDone)
{
	std::vector<SoundRegistry::Callback>& Waiting = InRegistry->Pending[InName];
	Waiting.push_back(InDone);
	if ( Waiting.size() > 1 )
	{
		return;
	}

	// The engine may call back at once (sound off, file missing), so nothing below uses Waiting.
	cocos2d::experimental::AudioEngine::preload(InPath, [InName, InPath, InRegistry](const bool bInSuccess)
	{
		model::Handle<SoundResource> Result;
		if ( bInSuccess )
		{
			const auto LoadedIt = InRegistry->Loaded.find(InName);
			Result = LoadedIt != InRegistry->Loaded.end() ? model::Handle<SoundResource>(LoadedIt->second)
				: model::Handle<SoundResource>(new SoundResource(InName, InPath, InRegistry));
		}
		std::vector<SoundRegistry::Callback> Done = std::move(InRegistry->Pending[InName]);
		InRegistry->Pending.erase(InName);
		for ( const SoundRegistry::Callback& CurrentCallback : Done )
		{
			CurrentCallback(Result);
		}
	});
}

SoundResource::SoundResource(const std::string& InName, const std::string& InPath, const std::shared_ptr<SoundRegistry>& InRegistry)
	: Name(InName), Path(InPath), Registry(InRegistry)
{
	Registry->Loaded[Name] = this;
}

SoundResource::~SoundResource()
{
	const auto LoadedIt = Registry->Loaded.find(Name);
	if ( LoadedIt != Registry->Loaded.end() && LoadedIt->second == this )
	{
		Registry->Loaded.erase(LoadedIt);
	}
	cocos2d::experimental::AudioEngine::uncache(Path);
}

const std::string& SoundResource::GetName() const
{
	return Name;
}

} // namespace tanks

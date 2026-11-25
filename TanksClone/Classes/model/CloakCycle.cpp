#include "model/CloakCycle.h"

#include <algorithm>

namespace game::model
{

void CloakCycle::Configure(const float InHiddenTime, const float InVisibleTime, const float InFadeTime, const float InHiddenOpacity)
{
	HiddenTime = std::max(0.0f, InHiddenTime);
	VisibleTime = std::max(0.0f, InVisibleTime);
	FadeTime = std::clamp(InFadeTime, 0.0f, std::min(HiddenTime, VisibleTime));
	HiddenOpacity = std::clamp(InHiddenOpacity, 0.0f, 1.0f);
	bHidden = false;
	Elapsed = FadeTime;
}

void CloakCycle::Update(const float InDelta)
{
	Elapsed += std::max(0.0f, InDelta);
	const float PartTime = bHidden ? HiddenTime : VisibleTime;
	if ( Elapsed >= PartTime )
	{
		Elapsed -= PartTime;
		bHidden = !bHidden;
	}
}

void CloakCycle::Reveal()
{
	bHidden = false;
	Elapsed = FadeTime;
}

bool CloakCycle::IsHidden() const
{
	return bHidden;
}

float CloakCycle::GetOpacity() const
{
	const float Fade = FadeTime > 0.0f ? std::min(1.0f, Elapsed / FadeTime) : 1.0f;
	const float HiddenShare = bHidden ? Fade : 1.0f - Fade;
	return 1.0f - (1.0f - HiddenOpacity) * HiddenShare;
}

} // namespace game::model

#pragma once

namespace tanks::model
{
/**
 * A vehicle that fades out of sight for a while, then shows itself again, over and over.
 * Being hit reveals it at once and starts the visible part anew.
 */
class CloakCycle
{
public:
	void Configure(float InHiddenTime, float InVisibleTime, float InFadeTime, float InHiddenOpacity);
	void Update(float InDelta);
	void Reveal();

	bool IsHidden() const;
	float GetOpacity() const;

private:
	float HiddenTime = 3.0f;
	float VisibleTime = 3.0f;
	float FadeTime = 0.4f;
	float HiddenOpacity = 0.1f;
	bool bHidden = false;
	float Elapsed = 0.0f;
};
} // namespace tanks::model

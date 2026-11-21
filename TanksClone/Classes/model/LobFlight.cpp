#include "model/LobFlight.h"

#include <algorithm>
#include <cmath>

namespace tanks::model
{

namespace
{
constexpr float GPi = 3.14159265358979323846f;
}

void LobFlight::Start(const float InDistance)
{
	Distance = std::max(0.0f, InDistance);
	Travelled = 0.0f;
	bFlying = true;
}

bool LobFlight::Advance(const float InStep)
{
	if ( !bFlying )
	{
		return false;
	}
	Travelled = std::min(Distance, Travelled + std::max(0.0f, InStep));
	if ( Travelled >= Distance )
	{
		bFlying = false;
		return true;
	}
	return false;
}

bool LobFlight::IsFlying() const
{
	return bFlying;
}

float LobFlight::GetProgress() const
{
	return Distance > 0.0f ? Travelled / Distance : 1.0f;
}

float LobFlight::GetDistance() const
{
	return Distance;
}

float LobFlight::GetHeight() const
{
	return std::sin(GetProgress() * GPi);
}

float LobFlight::ChooseDistance(const float InTargetDistance, const float InMin, const float InMax)
{
	if ( InTargetDistance <= 0.0f )
	{
		return InMax;
	}
	return std::clamp(InTargetDistance, InMin, InMax);
}

} // namespace tanks::model

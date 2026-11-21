#include "model/AiFeatureSet.h"

namespace tanks::model
{

namespace
{
	constexpr std::array<std::string_view, static_cast<size_t>(AiFeature::Count)> GNames =
	{
		"lead_shots", "dodge", "flank", "retreat", "guard_drops", "group"
	};
	static_assert(static_cast<size_t>(AiFeature::Group) + 1 == static_cast<size_t>(AiFeature::Count), "GNames follows AiFeature");
}

bool AiFeatureSet::Has(const AiFeature InFeature) const
{
	return InFeature < AiFeature::Count && Enabled[static_cast<size_t>(InFeature)];
}

void AiFeatureSet::Set(const AiFeature InFeature, const bool bInEnabled)
{
	if ( InFeature < AiFeature::Count )
	{
		Enabled[static_cast<size_t>(InFeature)] = bInEnabled;
	}
}

void AiFeatureSet::Apply(const AiSwitches& InSwitches)
{
	for ( const auto& Switch : InSwitches )
	{
		if ( const std::optional<AiFeature> Feature = FromName(Switch.first) )
		{
			Set(*Feature, Switch.second);
		}
	}
}

std::optional<AiFeature> AiFeatureSet::FromName(const std::string_view InName)
{
	for ( size_t Index = 0; Index < GNames.size(); ++Index )
	{
		if ( GNames[Index] == InName )
		{
			return static_cast<AiFeature>(Index);
		}
	}
	return std::nullopt;
}

AiSwitches AiFeatureSet::ReadSwitches(const IConfigNode& InNode)
{
	AiSwitches Switches;
	if ( const IConfigNode* const Node = InNode.FindChild("ai_features") )
	{
		for ( const std::string& Key : Node->GetKeys() )
		{
			Switches[Key] = Node->GetValueOr(Key, false);
		}
	}
	return Switches;
}

} // namespace tanks::model

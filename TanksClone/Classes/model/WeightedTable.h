#pragma once

#include <vector>

#include "model/Random.h"

namespace game::model
{
/**
 * A list of values with weights: Pick returns one at random, the heavier the likelier. Entries with a weight of zero
 * or less never get in. The generator is passed in, so a fixed seed gives the same picks.
 */
template <typename TValue>
class WeightedTable
{
public:
	void Clear()
	{
		Entries.clear();
		TotalWeight = 0;
	}

	void Add(const TValue& InValue, const int InWeight)
	{
		if ( InWeight <= 0 )
		{
			return;
		}
		Entries.push_back({ InValue, InWeight });
		TotalWeight += InWeight;
	}

	bool IsEmpty() const
	{
		return Entries.empty();
	}

	int GetTotalWeight() const
	{
		return TotalWeight;
	}

	const TValue* Pick(Random& InRandom) const
	{
		if ( Entries.empty() )
		{
			return nullptr;
		}
		int Rest = InRandom.Int(1, TotalWeight);
		for ( const Entry& Candidate : Entries )
		{
			Rest -= Candidate.Weight;
			if ( Rest <= 0 )
			{
				return &Candidate.Value;
			}
		}
		return &Entries.back().Value;
	}

private:
	struct Entry
	{
		TValue Value;
		int Weight = 1;
	};

	std::vector<Entry> Entries;
	int TotalWeight = 0;
};
} // namespace game::model

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/Random.h"

namespace tanks::model
{

/** One possible drop of a table: the pickup config and its weight among the others. */
struct DropEntry
{
	std::string Pickup;
	int Weight = 1;
};

/** A drop table: the chance of any drop, how much it grows after each miss, and what can drop. */
struct DropTable
{
	std::string Id;
	float Chance = 0.1f;
	float PityStep = 0.05f;
	std::vector<DropEntry> Entries;
};

/**
 * Decides what a killed enemy drops (configs/drops.json). Pseudo-random like PRD in MOBAs: every miss on a
 * table raises its chance by the pity step, a drop resets it, so long dry streaks don't happen.
 * The random generator is passed in, so a fixed seed gives the same drops.
 */
class DropRoller
{
public:
	// Reads a "tables" list of { "id", "chance", "pity_step", "entries": [ { "pickup", "weight" } ] }.
	void Load(const IConfigNode& InNode);
	void SetTables(const std::vector<DropTable>& InTables);
	// Forgets the misses, for a new level.
	void Reset();

	// The pickup to drop, or none. InChanceScale multiplies the chance (difficulty).
	std::optional<std::string> Roll(const std::string& InTableId, Random& InRandom, float InChanceScale = 1.0f);
	// The chance the next roll on this table has.
	float GetCurrentChance(const std::string& InTableId, float InChanceScale = 1.0f) const;

private:
	const DropTable* Find(const std::string& InTableId) const;

	std::vector<DropTable> Tables;
	std::map<std::string, int> Misses;
};

} // namespace tanks::model

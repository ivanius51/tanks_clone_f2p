#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "cocos2d.h"

#include "game/components/Stats.h"

namespace tanks
{

// A temporary or permanent effect on a tank, defined in configs/buffs.json:
// stat modifiers and/or invulnerability.
struct BuffDef
{
	std::string Id;
	std::string NameKey;           // string id for the HUD
	float Duration = -1.0f;        // seconds; negative = lasts until removed
	bool bInvulnerable = false;
	std::string Icon;              // sprite frame shown in the HUD
	cocos2d::Color3B IconColor = cocos2d::Color3B::WHITE;
	std::vector<StatModifier> Modifiers;
};

// All buffs the game knows.
class BuffCatalog
{
public:
	BuffCatalog() = default;
	~BuffCatalog() = default;
	BuffCatalog(const BuffCatalog&) = delete;
	BuffCatalog& operator=(const BuffCatalog&) = delete;

	// Call after the configs are loaded.
	void Init();
	// nullptr (and a log message) if there is no such buff.
	const BuffDef* Find(const std::string& InId) const;

private:
	std::map<std::string, BuffDef> Buffs;
};

// The buffs currently active on one tank.
class BuffSet
{
public:
	struct Active
	{
		BuffDef Def;
		float Remaining; // meaningless for permanent buffs
	};

	// Adding a buff that is already active only restarts its timer.
	void Add(const BuffDef& InDef);
	void Remove(std::string_view InId);

	// Counts time down. Returns true if a buff ran out.
	bool Update(float InDelta);

	std::vector<StatModifier> CollectModifiers() const;
	bool IsInvulnerable() const;
	const std::vector<Active>& GetActive() const;

private:
	std::vector<Active> ActiveBuffs;
};

} // namespace tanks

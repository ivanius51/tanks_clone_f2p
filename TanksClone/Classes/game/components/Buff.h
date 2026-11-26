#pragma once

#include <map>
#include <string>
#include <string_view>

#include "cocos2d.h"

#include "game/components/Stats.h"
#include "model/ConfigCatalog.h"
#include "templates/TContainers.h"

namespace game
{
/**
 * A temporary or permanent effect on a tank, defined in configs/balance/buffs.json:
 * stat modifiers and/or invulnerability.
 */
struct BuffDef
{
	FPersistentString Id;
	FPersistentString NameKey;
	float Duration = -1.0f;
	bool bInvulnerable = false;
	FPersistentString Icon;
	cocos2d::Color3B IconColor = cocos2d::Color3B::WHITE;
	TArray<StatModifier> Modifiers;
};

/** All buffs the game knows. */
class BuffCatalog
{
public:
	BuffCatalog() = default;
	~BuffCatalog() = default;
	BuffCatalog(const BuffCatalog&) = delete;
	BuffCatalog& operator=(const BuffCatalog&) = delete;

	void Init();
	const BuffDef* Find(const std::string& InId) const;

private:
	model::ConfigCatalog<BuffDef> Buffs;
};

/** The buffs currently active on one tank. */
class BuffSet
{
public:
	struct Active
	{
		BuffDef Def;
		float Remaining;
	};

	void Add(const BuffDef& InDef);
	void Remove(std::string_view InId);

	bool Update(float InDelta);

	TArray<StatModifier> CollectModifiers() const;
	bool IsInvulnerable() const;
	const TArray<Active>& GetActive() const;

private:
	TArray<Active> ActiveBuffs;
};
} // namespace game

#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"
#include "templates/TContainers.h"
#include "templates/TStoredString.h"

namespace game::model
{
/** One line of the upgrade tree: its nodes start at Anchor and go on by Step; the link to the first node starts at Origin (a part of the tank). */
struct UpgradeBranch
{
	FPersistentString Id;
	Vec2f Origin;
	Vec2f Anchor;
	Vec2f Step;
};

/** An upgrade placed on a branch; Parent is the upgrade it waits for (empty for none). */
struct UpgradeTreeNode
{
	FPersistentString Id;
	FPersistentString Branch;
	int Step = 0;
	FPersistentString Parent;
};

/** Places upgrades on their branches and links each to its parent, or to its branch origin when the parent is elsewhere. */
class UpgradeTree
{
public:
	static bool ReadBranch(const IConfigNode& InNode, UpgradeBranch& OutBranch);

	void SetBranches(const TArray<UpgradeBranch>& InBranches);
	TArray<FPersistentString> Build(const TArray<UpgradeTreeNode>& InNodes);

	int GetNodeCount() const;
	int FindNode(std::string_view InId) const;
	FPersistentString GetId(int InIndex) const;
	Vec2f GetPosition(int InIndex) const;
	Vec2f GetLinkStart(int InIndex) const;
	int GetParent(int InIndex) const;
	const TArray<Vec2f>& GetPositions() const;

private:
	const UpgradeBranch* FindBranch(std::string_view InId) const;

	TArray<UpgradeBranch> Branches;
	TArray<FPersistentString> Ids;
	TArray<Vec2f> Positions;
	TArray<Vec2f> Origins;
	TArray<int> Parents;
};
} // namespace game::model

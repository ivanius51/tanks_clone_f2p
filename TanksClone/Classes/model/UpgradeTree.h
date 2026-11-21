#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"

namespace tanks::model
{

/** One line of the upgrade tree: its nodes start at Anchor and go on by Step; the link to the first node starts at Origin (a part of the tank). */
struct UpgradeBranch
{
	std::string Id;
	Vec2f Origin;
	Vec2f Anchor;
	Vec2f Step;
};

/** An upgrade placed on a branch; Parent is the upgrade it waits for (empty for none). */
struct UpgradeTreeNode
{
	std::string Id;
	std::string Branch;
	int Step = 0;
	std::string Parent;
};

/** Places upgrades on their branches and links each to its parent, or to its branch origin when the parent is elsewhere. */
class UpgradeTree
{
public:
	// False when a field is missing.
	static bool ReadBranch(const IConfigNode& InNode, UpgradeBranch& OutBranch);

	void SetBranches(const std::vector<UpgradeBranch>& InBranches);
	// Nodes on an unknown branch are skipped and their ids returned.
	std::vector<std::string> Build(const std::vector<UpgradeTreeNode>& InNodes);

	int GetNodeCount() const;
	// -1 when the id is not in the tree.
	int FindNode(const std::string& InId) const;
	const std::string& GetId(int InIndex) const;
	Vec2f GetPosition(int InIndex) const;
	Vec2f GetLinkStart(int InIndex) const;
	// Index of the parent node, -1 when the link starts at the branch origin.
	int GetParent(int InIndex) const;
	const std::vector<Vec2f>& GetPositions() const;

private:
	const UpgradeBranch* FindBranch(const std::string& InId) const;

	std::vector<UpgradeBranch> Branches;
	std::vector<std::string> Ids;
	std::vector<Vec2f> Positions;
	std::vector<Vec2f> Origins;
	std::vector<int> Parents;
};

} // namespace tanks::model

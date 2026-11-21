#include "model/UpgradeTree.h"

namespace tanks::model
{

bool UpgradeTree::ReadBranch(const IConfigNode& InNode, UpgradeBranch& OutBranch)
{
	return InNode.TryRead("id", OutBranch.Id) && InNode.TryRead("origin", OutBranch.Origin) && InNode.TryRead("anchor", OutBranch.Anchor)
		&& InNode.TryRead("step", OutBranch.Step);
}

void UpgradeTree::SetBranches(const std::vector<UpgradeBranch>& InBranches)
{
	Branches = InBranches;
}

std::vector<std::string> UpgradeTree::Build(const std::vector<UpgradeTreeNode>& InNodes)
{
	Ids.clear();
	Positions.clear();
	Origins.clear();
	Parents.clear();
	std::vector<std::string> Skipped;
	std::vector<std::string> ParentIds;
	for ( const UpgradeTreeNode& Node : InNodes )
	{
		const UpgradeBranch* const Branch = FindBranch(Node.Branch);
		if ( !Branch )
		{
			Skipped.push_back(Node.Id);
			continue;
		}
		Ids.push_back(Node.Id);
		Positions.push_back(Branch->Anchor + Branch->Step * static_cast<float>(Node.Step));
		Origins.push_back(Branch->Origin);
		ParentIds.push_back(Node.Parent);
	}
	for ( std::size_t Index = 0; Index < Ids.size(); ++Index )
	{
		const int Parent = ParentIds[Index].empty() ? -1 : FindNode(ParentIds[Index]);
		const bool bSameBranch = Parent >= 0 && Origins[Parent] == Origins[Index];
		Parents.push_back(bSameBranch ? Parent : -1);
	}
	return Skipped;
}

int UpgradeTree::GetNodeCount() const
{
	return static_cast<int>(Ids.size());
}

int UpgradeTree::FindNode(const std::string& InId) const
{
	for ( std::size_t Index = 0; Index < Ids.size(); ++Index )
	{
		if ( Ids[Index] == InId )
		{
			return static_cast<int>(Index);
		}
	}
	return -1;
}

const std::string& UpgradeTree::GetId(const int InIndex) const
{
	return Ids[InIndex];
}

Vec2f UpgradeTree::GetPosition(const int InIndex) const
{
	return Positions[InIndex];
}

Vec2f UpgradeTree::GetLinkStart(const int InIndex) const
{
	return Parents[InIndex] >= 0 ? Positions[Parents[InIndex]] : Origins[InIndex];
}

int UpgradeTree::GetParent(const int InIndex) const
{
	return Parents[InIndex];
}

const std::vector<Vec2f>& UpgradeTree::GetPositions() const
{
	return Positions;
}

const UpgradeBranch* UpgradeTree::FindBranch(const std::string& InId) const
{
	for ( const UpgradeBranch& Branch : Branches )
	{
		if ( Branch.Id == InId )
		{
			return &Branch;
		}
	}
	return nullptr;
}

} // namespace tanks::model

#include "model/UpgradeTree.h"
#include "templates/TContainers.h"

namespace game::model
{

bool UpgradeTree::ReadBranch(const IConfigNode& InNode, UpgradeBranch& OutBranch)
{
	std::string IdText;
	if ( !InNode.TryRead("id", IdText) )
	{
		return false;
	}
	OutBranch.Id = FPersistentString{ IdText };
	return InNode.TryRead("origin", OutBranch.Origin) && InNode.TryRead("anchor", OutBranch.Anchor)
		&& InNode.TryRead("step", OutBranch.Step);
}

void UpgradeTree::SetBranches(const TArray<UpgradeBranch>& InBranches)
{
	Branches = InBranches;
}

TArray<FPersistentString> UpgradeTree::Build(const TArray<UpgradeTreeNode>& InNodes)
{
	Ids.Clear();
	Positions.Clear();
	Origins.Clear();
	Parents.Clear();
	TArray<FPersistentString> Skipped;
	TArray<FPersistentString> ParentIds;
	for ( const UpgradeTreeNode& Node : InNodes )
	{
		const UpgradeBranch* const Branch = FindBranch(Node.Branch.View());
		if ( !Branch )
		{
			Skipped.PushBack(Node.Id);
			continue;
		}
		Ids.PushBack(Node.Id);
		Positions.PushBack(Branch->Anchor + Branch->Step * static_cast<float>(Node.Step));
		Origins.PushBack(Branch->Origin);
		ParentIds.PushBack(Node.Parent);
	}
	for ( std::size_t Index = 0; Index < Ids.Size(); ++Index )
	{
		const int Parent = ParentIds[Index].IsEmpty() ? -1 : FindNode(ParentIds[Index].View());
		const bool bSameBranch = Parent >= 0 && Origins[Parent] == Origins[Index];
		Parents.PushBack(bSameBranch ? Parent : -1);
	}
	return Skipped;
}

int UpgradeTree::GetNodeCount() const
{
	return static_cast<int>(Ids.Size());
}

int UpgradeTree::FindNode(const std::string_view InId) const
{
	for ( std::size_t Index = 0; Index < Ids.Size(); ++Index )
	{
		if ( Ids[Index] == InId )
		{
			return static_cast<int>(Index);
		}
	}
	return -1;
}

FPersistentString UpgradeTree::GetId(const int InIndex) const
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

const TArray<Vec2f>& UpgradeTree::GetPositions() const
{
	return Positions;
}

const UpgradeBranch* UpgradeTree::FindBranch(const std::string_view InId) const
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

} // namespace game::model

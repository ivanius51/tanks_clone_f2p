#include "viewmodel/ViewDef.h"

namespace tanks::viewmodel
{

ViewDef ViewDef::FromConfig(const model::IConfigNode& InNode, const std::string& InId)
{
	ViewDef Result;
	Result.Id = InId;
	Result.Title = InNode.GetValueOr("title", InId.c_str());
	Result.Root = InNode.GetValueOr("root", "");
	Result.Menu = InNode.GetValueOr("menu", "");
	const model::IConfigNode* const TextsNode = InNode.FindChild("texts");
	for ( const std::string& Path : TextsNode ? TextsNode->GetKeys() : std::vector<std::string>() )
	{
		Result.Texts.emplace_back(Path, TextsNode->GetString(Path));
	}
	const model::IConfigNode* const ItemsNode = InNode.FindChild("items");
	for ( const std::string& Path : ItemsNode ? ItemsNode->GetKeys() : std::vector<std::string>() )
	{
		const model::IConfigNode* const ItemNode = ItemsNode->FindChild(Path);
		ViewItemDef Item;
		Item.Path = Path;
		Item.Widget = ItemNode->GetString("widget");
		const model::IConfigNode* const ItemTexts = ItemNode->FindChild("texts");
		for ( std::size_t Index = 0; ItemTexts && Index < ItemTexts->GetItemCount(); ++Index )
		{
			Item.Texts.push_back(ItemTexts->GetItem(Index)->ReadString().value_or(""));
		}
		Result.Items.push_back(std::move(Item));
	}
	return Result;
}

} // namespace tanks::viewmodel

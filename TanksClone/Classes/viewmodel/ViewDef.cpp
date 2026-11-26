#include "viewmodel/ViewDef.h"
#include "templates/TContainers.h"

namespace game::viewmodel
{

ViewDef ViewDef::FromConfig(const model::IConfigNode& InNode, const std::string& InId)
{
	ViewDef Result;
	Result.Id = InId;
	Result.Title = InNode.GetValueOr("title", InId.c_str());
	Result.Root = InNode.GetValueOr("root", "");
	Result.Menu = InNode.GetValueOr("menu", "");
	const model::IConfigNode* const TextsNode = InNode.FindChild("texts");
	for ( const std::string& Path : TextsNode ? TextsNode->GetKeys() : TArray<std::string>() )
	{
		Result.Texts.EmplaceBack(Path, TextsNode->GetString(Path));
	}
	const model::IConfigNode* const ItemsNode = InNode.FindChild("items");
	for ( const std::string& Path : ItemsNode ? ItemsNode->GetKeys() : TArray<std::string>() )
	{
		const model::IConfigNode* const ItemNode = ItemsNode->FindChild(Path);
		ViewItemDef Item;
		Item.Path = Path;
		Item.Widget = ItemNode->GetString("widget");
		const model::IConfigNode* const ItemTexts = ItemNode->FindChild("texts");
		for ( std::size_t Index = 0; ItemTexts && Index < ItemTexts->GetItemCount(); ++Index )
		{
			Item.Texts.PushBack(ItemTexts->GetItem(Index)->ReadString().value_or(""));
		}
		Result.Items.PushBack(std::move(Item));
	}
	return Result;
}

} // namespace game::viewmodel

#include "scenes/WidgetEditorScene.h"
#include "scenes/MenuScene.h"

#include <algorithm>
#include <string>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/LevelLoader.h"
#include "model/CustomLibrary.h"
#include "scenes/EditorScene.h"
#include "scenes/GameScene.h"
#include "scenes/LoadingScene.h"
#include "scenes/SceneUtils.h"
#include "scenes/SettingsScene.h"
#include "scenes/UpgradesScene.h"
#include "services/DifficultyManager.h"
#include "services/MusicManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/TankCondition.h"
#include "services/ResourceManager.h"
#include "game/ObjectsFactory.h"
#include "game/TankStateScript.h"
#include "game/objects/Tank.h"
#include "services/UiStyle.h"
#include "templates/TFunction.h"
#include "tests/Galleries.h"
#include "model/WidgetLayout.h"
#include "ui/LayoutWidget.h"
#include "ui/MenuBuilder.h"
#include "ui/WidgetFactory.h"
#include "ui/Widgets.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{

namespace
{
	int BestScoreFor(const PlayerProfile& InProfile)
	{
		auto* const Found = InProfile.BestScores.Find(GameContext::Get<DifficultyManager>().GetCurrent().Id);
		return Found ? *Found : 0;
	}

	constexpr int OverlayZ = 100;
	constexpr float GarageTankTiles = 1.1f;
	constexpr float GarageTankSize = 130.0f;

	std::string Text(const char* const InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
	}

	int NextLevelIndex()
	{
		const int Count = LevelLoader::GetBuiltinCount();
		return std::max(0, std::min(GameContext::Get<ProfileManager>().Get().LevelsCompleted, Count - 1));
	}

	LayoutWidget* FindContainer(LayoutWidget& InRoot, const std::string& InId)
	{
		if ( LayoutWidget* const Direct = InRoot.FindWidget(InId) )
		{
			return Direct;
		}
		for ( const char* const Host : { "panel", "side" } )
		{
			LayoutWidget* const Inner = InRoot.FindWidget(Host);
			if ( !Inner )
			{
				continue;
			}
			if ( LayoutWidget* const Found = Inner->FindWidget(InId) )
			{
				return Found;
			}
			LayoutWidget* const Garage = Inner->FindWidget("garage");
			if ( Garage && InId == "body" )
			{
				return Garage->FindWidget("body");
			}
			LayoutWidget* const Body = Garage ? Garage->FindWidget("body") : nullptr;
			if ( Body && Body->FindWidget(InId) )
			{
				return Body->FindWidget(InId);
			}
		}
		return nullptr;
	}
}

cocos2d::Scene* MenuScene::CreateScene()
{
	return MenuScene::create();
}

bool MenuScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}

	GameContext::Get<MusicManager>().Request(model::MusicMood::Menu);
	addChild(widgets::CreateControlHints(), 50);

	Group = WidgetFactory::Create(ids::widgets::MainMenu);
	const viewmodel::MenuDef* const Menu = GameContext::Get<UiStyle>().FindMenu(ids::menus::MainMenu);
	if ( !Group || !Menu )
	{
		return false;
	}
	addChild(Group);
	for ( const viewmodel::MenuTextDef& Current : Menu->Texts )
	{
		if ( !Current.Key.empty() )
		{
			Group->FindWidget("panel")->SetText(Current.Child, MenuText(Current));
		}
	}
	ShowStatus();

	const TankCondition Condition = TankCondition::ForLevel(NextLevelIndex());
	model::CustomLibrary Customs;
	Customs.Load(GameContext::Get().GetStorage());
	TArray<widgets::Overlay::ButtonSpec> CampaignButtons;
	for ( const model::CampaignDef& Campaign : Customs.GetCampaigns() )
	{
		if ( Campaign.LevelIds.IsEmpty() )
		{
			continue;
		}
		const std::string Id = Campaign.Id;
		const int Next = Customs.GetNextPosition(Id);
		CampaignButtons.PushBack({ Campaign.Name + "  " + std::to_string(Campaign.CompletedLevels) + "/" + std::to_string(Campaign.LevelIds.Size()),
			[Id, Next]() { SwitchScene(GameScene::CreateCampaignScene(Id, Next)); } });
	}
	CampaignButtons.PushBack({ Text("MENU_BACK"), []() { SwitchScene(MenuScene::CreateScene()); } });

	MenuBindings Bindings;
	Bindings.Conditions = {
		{ "has_progress", []() { return GameContext::Get<ProfileManager>().HasProgress(); } },
		{ "damaged", [Condition]() { return Condition.IsDamaged(); } },
		{ "can_repair", [Condition]() { return Condition.CanAffordRepair(); } }
	};
	Bindings.Values = {
		{ "next_level", []() { return Text("MENU_LEVEL") + " " + std::to_string(NextLevelIndex() + 1); } },
		{ "repair_cost", [Condition]() { return std::to_string(Condition.GetRepairCost()); } }
	};
	Bindings.Actions = {
		{ "play", [this]() { ContinueGame(); } },
		{ "new_game", [this]() { AskNewGame(); } },
		{ "upgrades", []() { SwitchScene(UpgradesScene::CreateScene()); } },
		{ "editor", []() { SwitchScene(EditorScene::CreateScene()); } },
		{ "custom", [this, CampaignButtons]() { ShowCustomMenu(CampaignButtons); } },
		{ "settings", []() { SwitchScene(SettingsScene::CreateScene()); } },
		{ "debug", [this]() { ShowDebugMenu(); } },
		{ "credits", [this]() { ShowCredits(); } },
		{ "repair", []()
			{
				if ( TankCondition::ForLevel(NextLevelIndex()).Repair() )
				{
					SwitchScene(MenuScene::CreateScene());
				}
			} },
		{ "quit", []() { cocos2d::Director::getInstance()->end(); } }
	};

	auto List = widgets::ButtonList::create();
	addChild(List);
	const cocos2d::Size DefaultSize = widgets::ScaleUi(cocos2d::Size(widgets::GetMenuButtonMetrics(Menu, "main").Width, widgets::GetMenuButtonMetrics(Menu, "main").Height));
	for ( const MenuEntry& Entry : ResolveMenuEntries(*Menu, Bindings, false) )
	{
		LayoutWidget* const Host = FindContainer(*Group, Entry.Item->Container.empty() ? Menu->Container : Entry.Item->Container);
		if ( !Host )
		{
			continue;
		}
		widgets::Button* const NewButton = widgets::CreateEntryButton(Entry, DefaultSize);
		if ( Entry.Item->Id == "repair" )
		{
			NewButton->SetText(Entry.Caption + "  {icon:bonus_coin} " + std::to_string(Condition.GetRepairCost()));
		}
		Host->AppendNode(NewButton);
		List->AddButton(NewButton);
	}

	if ( LayoutWidget* const Garage = FindContainer(*Group, "body") )
	{
		Garage->SetFlag("damaged", Condition.IsDamaged());
		Garage->SetPartFraction("durability", Condition.GetHealthShare());
		Group->Relayout();
		ShowGarageTank(*Garage, Condition);
		Garage->SetText("condition", Text("MENU_CONDITION") + ": " + std::to_string(static_cast<int>(std::lround(Condition.GetHealthShare() * 100))) + "%");
	}
	return true;
}

void MenuScene::ShowCustomMenu(const TArray<widgets::Overlay::ButtonSpec>& InCampaignButtons)
{
	const bool bHasCampaigns = InCampaignButtons.Size() > 1;
	MenuBindings Bindings;
	Bindings.Conditions = {
		{ "has_custom_level", []() { return LevelLoader::HasCustom(); } },
		{ "has_custom_campaigns", [bHasCampaigns]() { return bHasCampaigns; } }
	};
	Bindings.Actions = {
		{ "custom_level", []() { SwitchScene(GameScene::CreateCustomScene(GameScene::Origin::Menu)); } },
		{ "custom_campaigns", [this, InCampaignButtons]()
			{
				addChild(widgets::Overlay::create(Text("MENU_CUSTOM_CAMPAIGNS"), std::string(), InCampaignButtons), OverlayZ);
			} },
		{ "editor", []() { SwitchScene(EditorScene::CreateScene()); } },
		{ "back", []() { SwitchScene(MenuScene::CreateScene()); } }
	};
	addChild(widgets::Overlay::CreateFromMenu(ids::menus::Custom, Bindings), OverlayZ);
}

void MenuScene::ShowGarageTank(LayoutWidget& InGarage, const TankCondition& InCondition)
{
	cocos2d::Node* const Slot = InGarage.FindPart("tank");
	if ( !Slot )
	{
		return;
	}
	const cocos2d::Vec2 Center = Slot->getParent()->convertToWorldSpace(Slot->getPosition());
	GarageStage = cocos2d::Node::create();
	GarageStage->setPosition(convertToNodeSpace(Center));
	GarageStage->setScale(GarageTankSize * widgets::GetUiScale() / (TileSize * GarageTankTiles));
	addChild(GarageStage, 1);
	model::JsonValue::Object Overrides;
	Overrides.InsertOrAssign("position", model::JsonValue(model::Vec2f{ 0.0f, 0.0f }));
	GarageTank = GameContext::Get<ObjectsFactory>().CreateFromConfig<Tank>(GameContext::Get<ResourceManager>().ResolvePath(ids::configs::PlayerTank, model::ResourceKind::Config).View(), GarageStage, Overrides);
	if ( !GarageTank )
	{
		return;
	}
	GarageTank->SetFrozen(true);
	TankStateScript::Apply(*GarageTank, TankStateScript::ForHealthShare(InCondition.GetHealthShare()));
}

void MenuScene::onExit()
{
	if ( GarageTank )
	{
		GarageTank->Destroy();
		GarageTank = FTankHandle();
	}
	Scene::onExit();
}

void MenuScene::ShowStatus()
{
	const auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
	LayoutWidget* const Wallet = FindContainer(*Group, "wallet");
	if ( !Wallet )
	{
		return;
	}
	Wallet->SetText("coins", "{icon:bonus_coin} " + std::to_string(CurrentProfile.Coins));
	Wallet->SetText("score", Text("HUD_SCORE") + ": " + std::to_string(BestScoreFor(CurrentProfile)));
}

void MenuScene::ShowCredits()
{
	TArray<widgets::Overlay::ButtonSpec> Buttons;
	Buttons.PushBack({ Text("MENU_BACK"), []() { SwitchScene(MenuScene::CreateScene()); } });
	addChild(widgets::Overlay::create(Text("MENU_CREDITS"), Text("MENU_CREDITS_TEXT"), Buttons), OverlayZ);
}

void MenuScene::AskNewGame()
{
	if ( !GameContext::Get<ProfileManager>().HasProgress() )
	{
		StartNewGame();
		return;
	}
	MenuBindings Bindings;
	Bindings.Actions = {
		{ "yes", [this]() { StartNewGame(); } },
		{ "no", []() { SwitchScene(MenuScene::CreateScene()); } }
	};
	addChild(widgets::Overlay::CreateFromMenu(ids::menus::ConfirmNewGame, Bindings), OverlayZ);
}

void MenuScene::ShowDebugMenu()
{
	MenuBindings Bindings;
	Bindings.Actions = {
		{ "galleries", [this]() { ShowGalleries(); } },
		{ "widget_editor", []() { SwitchScene(WidgetEditorScene::CreateScene()); } },
		{ "back", []() { SwitchScene(MenuScene::CreateScene()); } }
	};
	addChild(widgets::Overlay::CreateFromMenu(ids::menus::Debug, Bindings), OverlayZ);
}

void MenuScene::ShowGalleries()
{
	TArray<widgets::Overlay::ButtonSpec> Buttons;
	for ( const tests::GalleryEntry& Entry : tests::Galleries::GetAll() )
	{
		const std::string Id = Entry.Id;
		Buttons.PushBack({ Text(Entry.NameKey.c_str()), [Id]() { tests::Galleries::Play(Id); } });
	}
	Buttons.PushBack({ Text("BACK_DEBUG"), []() { SwitchScene(MenuScene::CreateScene()); } });
	addChild(widgets::Overlay::create(Text("GALLERIES_TITLE"), Text("GALLERIES_TEXT"), Buttons), OverlayZ);
}

void MenuScene::StartNewGame()
{
	GameContext::Get<ProfileManager>().ResetProgress();
	GameContext::Get<ProfileManager>().Save();
	SwitchScene(LoadingScene::CreateForLevel(0));
}

void MenuScene::ContinueGame()
{
	SwitchScene(LoadingScene::CreateForLevel(NextLevelIndex()));
}

} // namespace game::view::cocos

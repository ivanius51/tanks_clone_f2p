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
	const viewmodel::MainMenuLayout& Layout()
	{
		return GameContext::Get<UiStyle>().GetMenu().Main;
	}

	viewmodel::MenuButtonMetrics Buttons()
	{
		return widgets::GetMenuButtonMetrics(GameContext::Get<UiStyle>().FindMenu(ids::menus::MainMenu), "main");
	}

	int BestScoreFor(const PlayerProfile& InProfile)
	{
		auto* const Found = InProfile.BestScores.Find(GameContext::Get<DifficultyManager>().GetCurrent().Id);
		return Found ? *Found : 0;
	}


	constexpr int OverlayZ = 100;

	std::string Text(const char* const InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
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

	const auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();

	GameContext::Get<MusicManager>().Request(model::MusicMood::Menu);
	addChild(widgets::CreateScreenBackground());
	addChild(widgets::CreateControlHints(), 50);

	Group = WidgetFactory::Create(ids::widgets::MainMenu);
	if ( !Group )
	{
		return false;
	}
	addChild(Group);
	const viewmodel::MenuDef* const Menu = GameContext::Get<UiStyle>().FindMenu(ids::menus::MainMenu);
	if ( !Menu )
	{
		return false;
	}
	for ( const viewmodel::MenuTextDef& Current : Menu->Texts )
	{
		if ( !Current.Key.empty() )
		{
			Group->SetText(Current.Child, MenuText(Current));
		}
	}
	Group->SetText("info", Text("HUD_SCORE") + ": " + std::to_string(BestScoreFor(CurrentProfile))
		+ "     " + Text("UPGRADES_COINS") + ": " + std::to_string(CurrentProfile.Coins));

	auto List = widgets::ButtonList::create();
	addChild(List);

	auto CurrentAddButton = [this, List](const std::string& InText, const TFunction<void(), UiCallbackInlineSize>& InCallback)
	{
		auto CurrentButton = widgets::Button::create(InText, widgets::ScaleUi(cocos2d::Size(Buttons().Width, Buttons().Height)), InCallback);
		Group->FindWidget("buttons")->AppendNode(CurrentButton);
		List->AddButton(CurrentButton);
	};

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
	const bool bHasCampaigns = CampaignButtons.Size() > 1;

	MenuBindings Bindings;
	Bindings.Conditions = {
		{ "has_progress", []() { return GameContext::Get<ProfileManager>().HasProgress(); } },
		{ "has_custom_level", []() { return LevelLoader::HasCustom(); } },
		{ "has_custom_campaigns", [bHasCampaigns]() { return bHasCampaigns; } }
	};
	Bindings.Actions = {
		{ "continue", [this]() { ContinueGame(); } },
		{ "new_game", [this]() { AskNewGame(); } },
		{ "upgrades", []() { SwitchScene(UpgradesScene::CreateScene()); } },
		{ "editor", []() { SwitchScene(EditorScene::CreateScene()); } },
		{ "custom_level", []() { SwitchScene(GameScene::CreateCustomScene(GameScene::Origin::Menu)); } },
		{ "custom_campaigns", [this, CampaignButtons]()
			{
				addChild(widgets::Overlay::create(Text("MENU_CUSTOM_CAMPAIGNS"), std::string(), CampaignButtons), OverlayZ);
			} },
		{ "settings", []() { SwitchScene(SettingsScene::CreateScene()); } },
		{ "debug", [this]() { ShowDebugMenu(); } },
		{ "quit", []() { cocos2d::Director::getInstance()->end(); } }
	};
	for ( const MenuEntry& Entry : ResolveMenuEntries(*Menu, Bindings, false) )
	{
		CurrentAddButton(Entry.Caption, Entry.Action);
	}

	FitGroup();
	return true;
}

void MenuScene::FitGroup()
{
	const float TextScale = widgets::GetUiScale();
	const float Margin = Layout().ScreenMargin;
	LayoutWidget* const Column = Group->FindWidget("buttons");
	const float Height = Buttons().Height * TextScale;
	const float Spacing = Buttons().Spacing * TextScale;
	const float Count = static_cast<float>(Column->getChildrenCount());
	const float FullListHeight = Count * (Height + Spacing) - Spacing;
	const auto ApplyHeights = [Column](const float InHeight, const float InSpacing)
	{
		for ( cocos2d::Node* const Current : Column->getChildren() )
		{
			widgets::Button::FromNode(Current)->SetSize(cocos2d::Size(Buttons().Width * widgets::GetUiScale(), InHeight));
		}
		Column->SetGap(InSpacing);
		Column->Relayout();
	};
	ApplyHeights(Height, Spacing);
	const float FixedHeight = Group->getContentSize().height - FullListHeight;
	const float Factor = model::widget_layout::FitFactor(DesignHeight - 2 * Margin, FixedHeight, FullListHeight, Layout().MinFitFactor);
	ApplyHeights(Height * Factor, Spacing * Factor);

	const float GroupHeight = Group->getContentSize().height;
	const float CenterY = std::clamp(DesignHeight / 2 + Layout().ListOffsetY, Margin + GroupHeight / 2, DesignHeight - Margin - GroupHeight / 2);
	Group->setPosition(DesignWidth / 2, CenterY);
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
	const int CurrentCount = LevelLoader::GetBuiltinCount();
	const int CurrentLevel = std::min(GameContext::Get<ProfileManager>().Get().LevelsCompleted, CurrentCount - 1);
	SwitchScene(LoadingScene::CreateForLevel(std::max(0, CurrentLevel)));
}

} // namespace game::view::cocos

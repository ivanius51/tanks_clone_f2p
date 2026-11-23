#include "scenes/WidgetEditorScene.h"
#include "scenes/MenuScene.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

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
#include "tests/Galleries.h"
#include "model/WidgetLayout.h"
#include "ui/LayoutWidget.h"
#include "ui/WidgetFactory.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	const viewmodel::MainMenuLayout& Layout()
	{
		return GameContext::Get().GetManager<UiStyle>().GetMenu().Main;
	}

	int BestScoreFor(const PlayerProfile& InProfile)
	{
		const auto Found = InProfile.BestScores.find(GameContext::Get().GetManager<DifficultyManager>().GetCurrent().Id);
		return Found != InProfile.BestScores.end() ? Found->second : 0;
	}


	constexpr int OverlayZ = 100;

	std::string Text(const char* const InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
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

	const auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();

	GameContext::Get().GetManager<MusicManager>().Request(model::MusicMood::Menu);
	addChild(widgets::CreateScreenBackground());
	addChild(widgets::CreateControlHints(), 50);

	Group = WidgetFactory::Create(ids::widgets::MainMenu);
	if ( !Group )
	{
		return false;
	}
	addChild(Group);
	Group->SetText("title", Text("GAME_TITLE"));
	Group->SetText("info", Text("HUD_SCORE") + ": " + std::to_string(BestScoreFor(CurrentProfile))
		+ "     " + Text("UPGRADES_COINS") + ": " + std::to_string(CurrentProfile.Coins));

	auto List = widgets::ButtonList::create();
	addChild(List);

	auto CurrentAddButton = [this, List](const std::string& InText, const std::function<void()>& InCallback)
	{
		auto CurrentButton = widgets::Button::create(InText, widgets::ScaleUi(cocos2d::Size(Layout().ButtonWidth, Layout().ButtonHeight)), InCallback);
		Group->FindWidget("buttons")->AppendNode(CurrentButton);
		List->AddButton(CurrentButton);
	};

	if ( GameContext::Get().GetManager<ProfileManager>().HasProgress() )
	{
		CurrentAddButton(Text("MENU_CONTINUE"), [this]() { ContinueGame(); });
	}
	CurrentAddButton(Text("MENU_NEW_GAME"), [this]()
	{
		if ( !GameContext::Get().GetManager<ProfileManager>().HasProgress() )
		{
			StartNewGame();
			return;
		}

		auto CurrentOverlay = widgets::Overlay::create(Text("CONFIRM_NEW_GAME_TITLE"), Text("CONFIRM_NEW_GAME_TEXT"), {
			{ Text("YES"), [this]() { StartNewGame(); } },
			{ Text("NO"), []() { SwitchScene(MenuScene::CreateScene()); } }
		});
		addChild(CurrentOverlay, OverlayZ);
	});
	CurrentAddButton(Text("MENU_UPGRADES"), []() { SwitchScene(UpgradesScene::CreateScene()); });
	CurrentAddButton(Text("MENU_EDITOR"), []() { SwitchScene(EditorScene::CreateScene()); });
	if ( LevelLoader::HasCustom() )
	{
		CurrentAddButton(Text("MENU_CUSTOM_LEVEL"), []() { SwitchScene(GameScene::CreateCustomScene(GameScene::Origin::Menu)); });
	}
	model::CustomLibrary Customs;
	Customs.Load(GameContext::Get().GetStorage());
	std::vector<widgets::Overlay::ButtonSpec> CampaignButtons;
	for ( const model::CampaignDef& Campaign : Customs.GetCampaigns() )
	{
		if ( Campaign.LevelIds.empty() )
		{
			continue;
		}
		const std::string Id = Campaign.Id;
		const int Next = Customs.GetNextPosition(Id);
		CampaignButtons.push_back({ Campaign.Name + "  " + std::to_string(Campaign.CompletedLevels) + "/" + std::to_string(Campaign.LevelIds.size()),
			[Id, Next]() { SwitchScene(GameScene::CreateCampaignScene(Id, Next)); } });
	}
	if ( !CampaignButtons.empty() )
	{
		CampaignButtons.push_back({ Text("MENU_BACK"), []() { SwitchScene(MenuScene::CreateScene()); } });
		CurrentAddButton(Text("MENU_CUSTOM_CAMPAIGNS"), [this, CampaignButtons]()
		{
			addChild(widgets::Overlay::create(Text("MENU_CUSTOM_CAMPAIGNS"), std::string(), CampaignButtons), OverlayZ);
		});
	}
	CurrentAddButton(Text("MENU_SETTINGS"), []() { SwitchScene(SettingsScene::CreateScene()); });
	if constexpr ( bHasDebugTools )
	{
		CurrentAddButton(Text("MENU_DEBUG"), [this]() { ShowDebugMenu(); });
	}
	CurrentAddButton(Text("MENU_QUIT"), []() { cocos2d::Director::getInstance()->end(); });

	FitGroup();
	return true;
}

void MenuScene::FitGroup()
{
	const float TextScale = widgets::GetUiScale();
	const float Margin = Layout().ScreenMargin;
	LayoutWidget* const Column = Group->FindWidget("buttons");
	const float Height = Layout().ButtonHeight * TextScale;
	const float Spacing = Layout().ButtonSpacing * TextScale;
	const float Count = static_cast<float>(Column->getChildrenCount());
	const float FullListHeight = Count * (Height + Spacing) - Spacing;
	const auto ApplyHeights = [Column](const float InHeight, const float InSpacing)
	{
		for ( cocos2d::Node* const Current : Column->getChildren() )
		{
			widgets::Button::FromNode(Current)->SetSize(cocos2d::Size(Layout().ButtonWidth * widgets::GetUiScale(), InHeight));
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

void MenuScene::ShowDebugMenu()
{
	addChild(widgets::Overlay::create(Text("DEBUG_TITLE"), Text("DEBUG_TEXT"), {
		{ Text("DEBUG_GALLERIES"), [this]() { ShowGalleries(); } },
		{ Text("DEBUG_WIDGET_EDITOR"), []() { SwitchScene(WidgetEditorScene::CreateScene()); } },
		{ Text("BACK_DEBUG"), []() { SwitchScene(MenuScene::CreateScene()); } }
	}), OverlayZ);
}

void MenuScene::ShowGalleries()
{
	std::vector<widgets::Overlay::ButtonSpec> Buttons;
	for ( const tests::GalleryEntry& Entry : tests::Galleries::GetAll() )
	{
		const std::string Id = Entry.Id;
		Buttons.push_back({ Text(Entry.NameKey.c_str()), [Id]() { tests::Galleries::Play(Id); } });
	}
	Buttons.push_back({ Text("BACK_DEBUG"), []() { SwitchScene(MenuScene::CreateScene()); } });
	addChild(widgets::Overlay::create(Text("GALLERIES_TITLE"), Text("GALLERIES_TEXT"), Buttons), OverlayZ);
}

void MenuScene::StartNewGame()
{
	GameContext::Get().GetManager<ProfileManager>().ResetProgress();
	GameContext::Get().GetManager<ProfileManager>().Save();
	SwitchScene(LoadingScene::CreateForLevel(0));
}

void MenuScene::ContinueGame()
{
	const int CurrentCount = LevelLoader::GetBuiltinCount();
	const int CurrentLevel = std::min(GameContext::Get().GetManager<ProfileManager>().Get().LevelsCompleted, CurrentCount - 1);
	SwitchScene(LoadingScene::CreateForLevel(std::max(0, CurrentLevel)));
}

} // namespace tanks::view::cocos

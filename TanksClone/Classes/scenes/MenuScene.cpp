#include "scenes/MenuScene.h"

#include <algorithm>
#include <functional>
#include <string>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/LevelLoader.h"
#include "scenes/EditorScene.h"
#include "scenes/GameScene.h"
#include "scenes/LoadingScene.h"
#include "scenes/SceneUtils.h"
#include "scenes/SettingsScene.h"
#include "scenes/UpgradesScene.h"
#include "services/DifficultyManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "tests/Galleries.h"
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

	addChild(widgets::CreateScreenBackground());

	auto Title = widgets::CreateLabel(Text("GAME_TITLE"), Layout().TitleFontSize, widgets::colors::Accent(), true);
	Title->setPosition(DesignWidth / 2, DesignHeight - Layout().TitleTop);
	addChild(Title);

	auto Info = widgets::CreateLabel(Text("HUD_SCORE") + ": " + std::to_string(BestScoreFor(CurrentProfile))
		+ "     " + Text("UPGRADES_COINS") + ": " + std::to_string(CurrentProfile.Coins), Layout().InfoFontSize, widgets::colors::TextDim());
	Info->setPosition(DesignWidth / 2, DesignHeight - Layout().InfoTop);
	addChild(Info);

	auto List = widgets::ButtonList::create();
	addChild(List);

	auto CurrentAddButton = [this, List](const std::string& InText, const std::function<void()>& InCallback)
	{
		auto CurrentButton = widgets::Button::create(InText, widgets::ScaleUi(cocos2d::Size(Layout().ButtonWidth, Layout().ButtonHeight)), InCallback);
		addChild(CurrentButton, 1);
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
			{ Text("NO"), [this]() { SwitchScene(MenuScene::CreateScene()); } }
		});
		addChild(CurrentOverlay, OverlayZ);
	});
	CurrentAddButton(Text("MENU_UPGRADES"), []() { SwitchScene(UpgradesScene::CreateScene()); });
	CurrentAddButton(Text("MENU_EDITOR"), []() { SwitchScene(EditorScene::CreateScene()); });
	if ( LevelLoader::HasCustom() )
	{
		CurrentAddButton(Text("MENU_CUSTOM_LEVEL"), []() { SwitchScene(GameScene::CreateCustomScene(GameScene::Origin::Menu)); });
	}
	CurrentAddButton(Text("MENU_SETTINGS"), []() { SwitchScene(SettingsScene::CreateScene()); });
	if constexpr ( bHasDebugTools )
	{
		CurrentAddButton(Text("MENU_DEBUG"), [this]() { ShowDebugMenu(); });
	}
	CurrentAddButton(Text("MENU_QUIT"), []() { cocos2d::Director::getInstance()->end(); });

	List->LayoutVertically(cocos2d::Vec2(DesignWidth / 2, DesignHeight / 2 + Layout().ListOffsetY), Layout().ButtonSpacing * widgets::GetUiScale());
	return true;
}

void MenuScene::ShowDebugMenu()
{
	addChild(widgets::Overlay::create(Text("DEBUG_TITLE"), Text("DEBUG_TEXT"), {
		{ Text("DEBUG_GALLERIES"), [this]() { ShowGalleries(); } },
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

#include "scenes/SettingsScene.h"

#include <functional>
#include <string>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr float TitleFontSize = 56.0f;
	constexpr float ButtonWidth = 560.0f;
	constexpr float ButtonHeight = 48.0f;
	constexpr float ButtonSpacing = 8.0f;
	constexpr float ListCenterOffset = 40.0f;
	constexpr int VolumeStep = 20;

	std::string Text(const char* const InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}
}

cocos2d::Scene* SettingsScene::CreateScene()
{
	return SettingsScene::create();
}

bool SettingsScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}
	Build();
	return true;
}

void SettingsScene::Build()
{
	removeAllChildren();
	addChild(widgets::CreateScreenBackground());

	auto Title = widgets::CreateLabel(Text("SETTINGS_TITLE"), TitleFontSize, widgets::colors::Accent, true);
	Title->setPosition(DesignWidth / 2, DesignHeight - 120);
	addChild(Title);

	auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
	auto List = widgets::ButtonList::create();
	addChild(List);

	auto CurrentAddButton = [this, List](const std::string& InText, const std::function<void()>& InCallback)
	{
		auto CurrentButton = widgets::Button::create(InText, cocos2d::Size(ButtonWidth, ButtonHeight), InCallback);
		addChild(CurrentButton, 1);
		List->AddButton(CurrentButton);
	};

	// The buttons rebuild the whole screen, so do that on the next frame, when the
	// button that was pressed is no longer running its callback.
	auto RebuildLater = [this]()
	{
		scheduleOnce([this](float) { Build(); }, 0.0f, "rebuild");
	};

	CurrentAddButton(Text("SETTINGS_LANGUAGE") + ": " + Text("LANGUAGE_NAME"), [this, RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		switch ( CurrentSettings.CurrentLanguage )
		{
			case Language::English: CurrentSettings.CurrentLanguage = Language::Ukrainian; break;
			case Language::Ukrainian: CurrentSettings.CurrentLanguage = Language::Russian; break;
			default: CurrentSettings.CurrentLanguage = Language::English; break;
		}
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	CurrentAddButton(Text("SETTINGS_SOUND") + ": " + Text(CurrentProfile.bSoundEnabled ? "ON" : "OFF"), [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.bSoundEnabled = !CurrentSettings.bSoundEnabled;
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	CurrentAddButton(Text("SETTINGS_EFFECTS_VOLUME") + ": " + std::to_string(CurrentProfile.EffectsVolume) + "%", [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.EffectsVolume = CurrentSettings.EffectsVolume <= 0 ? 100 : CurrentSettings.EffectsVolume - VolumeStep;
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	const std::string Fps = CurrentProfile.FpsLimit > 0 ? std::to_string(CurrentProfile.FpsLimit) : Text("FPS_UNLIMITED");
	CurrentAddButton(Text("SETTINGS_FPS") + ": " + Fps, [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.FpsLimit = NextFpsOption(CurrentSettings.FpsLimit);
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	CurrentAddButton(Text("SETTINGS_VSYNC") + ": " + Text(CurrentProfile.bVerticalSync ? "ON" : "OFF"), [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.bVerticalSync = !CurrentSettings.bVerticalSync;
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	const bool bBorderless = CurrentProfile.CurrentWindowMode == WindowMode::Borderless;
	CurrentAddButton(Text("SETTINGS_WINDOW_MODE") + ": " + Text(bBorderless ? "WINDOW_MODE_BORDERLESS" : "WINDOW_MODE_WINDOWED"), [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.CurrentWindowMode = CurrentSettings.CurrentWindowMode == WindowMode::Borderless ? WindowMode::Windowed : WindowMode::Borderless;
		GameContext::Get().GetManager<ProfileManager>().ApplyWindowSettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	const std::string Scale = CurrentProfile.WindowScale > 0 ? std::to_string(CurrentProfile.WindowScale) + "%" : Text("WINDOW_SIZE_AUTO");
	CurrentAddButton(Text("SETTINGS_WINDOW_SIZE") + ": " + Scale, [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.WindowScale = NextWindowScale(CurrentSettings.WindowScale);
		GameContext::Get().GetManager<ProfileManager>().ApplyWindowSettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	CurrentAddButton(Text("SETTINGS_PIXELS") + ": " + Text(CurrentProfile.bSmoothPixels ? "PIXELS_SMOOTH" : "PIXELS_SHARP"), [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.bSmoothPixels = !CurrentSettings.bSmoothPixels;
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	CurrentAddButton(Text("SETTINGS_TEXTURES") + ": " + Text(CurrentProfile.bHdTextures ? "TEXTURES_HD" : "TEXTURES_SD"), [RebuildLater]()
	{
		auto& CurrentSettings = GameContext::Get().GetManager<ProfileManager>().Get();
		CurrentSettings.bHdTextures = !CurrentSettings.bHdTextures;
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ProfileManager>().Save();
		RebuildLater();
	});

	CurrentAddButton(Text("MENU_BACK"), []() { SwitchScene(MenuScene::CreateScene()); });

	List->LayoutVertically(cocos2d::Vec2(DesignWidth / 2, DesignHeight / 2 - ListCenterOffset), ButtonSpacing);
}

} // namespace tanks::view::cocos

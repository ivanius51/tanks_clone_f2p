#include "services/UiStyle.h"
#include "ui/SettingsPanel.h"

#include <algorithm>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "services/DifficultyManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr int MaxVolume = 100;

	const viewmodel::SettingsLayout& Style()
	{
		return GameContext::Get().GetManager<UiStyle>().GetMenu().Settings;
	}


	std::string Text(const char* const InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}

	PlayerProfile& Profile()
	{
		return GameContext::Get().GetManager<ProfileManager>().Get();
	}

	const char* TextSizeName(const model::TextSize InSize)
	{
		switch ( InSize )
		{
			case model::TextSize::Medium: return "TEXT_SIZE_MEDIUM";
			case model::TextSize::Large: return "TEXT_SIZE_LARGE";
			default: return "TEXT_SIZE_SMALL";
		}
	}

	Language NextLanguage(const Language InLanguage)
	{
		switch ( InLanguage )
		{
			case Language::English: return Language::Ukrainian;
			case Language::Ukrainian: return Language::Russian;
			default: return Language::English;
		}
	}
}

SettingsPanel* SettingsPanel::create(const Place InPlace, const std::function<void()>& InOnBack, const std::function<void()>& InOnReopen)
{
	SettingsPanel* NewPanel = new (std::nothrow) SettingsPanel();
	if ( NewPanel && NewPanel->init(InPlace, InOnBack, InOnReopen) )
	{
		NewPanel->autorelease();
		return NewPanel;
	}
	CC_SAFE_DELETE(NewPanel);
	return nullptr;
}

bool SettingsPanel::init(const Place InPlace, const std::function<void()>& InOnBack, const std::function<void()>& InOnReopen)
{
	if ( !Node::init() )
	{
		return false;
	}
	Where = InPlace;
	Scale = widgets::GetUiScale();

	Title = widgets::CreateLabel(Text("SETTINGS_TITLE"), Style().TitleFontSize, widgets::colors::Accent(), true);
	addChild(Title, 1);
	List = widgets::ButtonList::create();
	List->SetBelongsToOverlay(Where == Place::Level);
	addChild(List);

	AddSetting([]() { return Text("SETTINGS_LANGUAGE") + ": " + Text("LANGUAGE_NAME"); },
		[](PlayerProfile& InProfile) { InProfile.CurrentLanguage = NextLanguage(InProfile.CurrentLanguage); });
	AddEntry([]() { return Text("SETTINGS_TEXT_SIZE") + ": " + Text(TextSizeName(Profile().CurrentTextSize)); }, [this, InOnReopen]()
	{
		Apply([](PlayerProfile& InProfile) { InProfile.CurrentTextSize = model::TextScales::Next(InProfile.CurrentTextSize); }, false);
		InOnReopen();
	});
	AddSetting([]() { return Text("SETTINGS_DIFFICULTY") + ": " + Text(GameContext::Get().GetManager<DifficultyManager>().GetCurrent().NameKey.c_str()); },
		[](PlayerProfile&) { GameContext::Get().GetManager<DifficultyManager>().SelectNext(); }, false, Where == Place::Screen);
	AddSetting([]() { return Text("SETTINGS_SOUND") + ": " + Text(Profile().bSoundEnabled ? "ON" : "OFF"); },
		[](PlayerProfile& InProfile) { InProfile.bSoundEnabled = !InProfile.bSoundEnabled; });
	AddSetting([]() { return Text("SETTINGS_EFFECTS_VOLUME") + ": " + std::to_string(Profile().EffectsVolume) + "%"; },
		[](PlayerProfile& InProfile) { InProfile.EffectsVolume = InProfile.EffectsVolume <= 0 ? MaxVolume : std::max(InProfile.EffectsVolume - Style().VolumeStep, 0); });
	Entries.back().Target->SetAdjust([this](const int InDirection)
	{
		Apply([InDirection](PlayerProfile& InProfile)
		{
			InProfile.EffectsVolume = std::clamp(InProfile.EffectsVolume + InDirection * Style().VolumeStep, 0, MaxVolume);
		}, false);
	});
	AddSetting([]() { return Text("SETTINGS_FPS") + ": " + (Profile().FpsLimit > 0 ? std::to_string(Profile().FpsLimit) : Text("FPS_UNLIMITED")); },
		[](PlayerProfile& InProfile) { InProfile.FpsLimit = NextFpsOption(InProfile.FpsLimit); });
	AddSetting([]() { return Text("SETTINGS_VSYNC") + ": " + Text(Profile().bVerticalSync ? "ON" : "OFF"); },
		[](PlayerProfile& InProfile) { InProfile.bVerticalSync = !InProfile.bVerticalSync; });
	AddSetting([]()
		{
			const bool bBorderless = Profile().CurrentWindowMode == WindowMode::Borderless;
			return Text("SETTINGS_WINDOW_MODE") + ": " + Text(bBorderless ? "WINDOW_MODE_BORDERLESS" : "WINDOW_MODE_WINDOWED");
		},
		[](PlayerProfile& InProfile)
		{
			InProfile.CurrentWindowMode = InProfile.CurrentWindowMode == WindowMode::Borderless ? WindowMode::Windowed : WindowMode::Borderless;
		}, true);
	AddSetting([]() { return Text("SETTINGS_WINDOW_SIZE") + ": " + (Profile().WindowScale > 0 ? std::to_string(Profile().WindowScale) + "%" : Text("WINDOW_SIZE_AUTO")); },
		[](PlayerProfile& InProfile) { InProfile.WindowScale = NextWindowScale(InProfile.WindowScale); }, true);
	AddSetting([]() { return Text("SETTINGS_PIXELS") + ": " + Text(Profile().bSmoothPixels ? "PIXELS_SMOOTH" : "PIXELS_SHARP"); },
		[](PlayerProfile& InProfile) { InProfile.bSmoothPixels = !InProfile.bSmoothPixels; });
	AddSetting([]() { return Text("SETTINGS_TEXTURES") + ": " + Text(Profile().bHdTextures ? "TEXTURES_HD" : "TEXTURES_SD"); },
		[](PlayerProfile& InProfile) { InProfile.bHdTextures = !InProfile.bHdTextures; }, false, Where == Place::Screen);
	TextureButton = Entries.back().Target;
	AddEntry([]() { return Text("MENU_BACK"); }, InOnBack);
	Layout();
	return true;
}

void SettingsPanel::Layout()
{
	const float Spacing = Style().ButtonSpacing * Scale;
	const float ListHeight = Entries.size() * (Style().ButtonHeight * Scale + Spacing) - Spacing;
	const float TitleHeight = Title->getContentSize().height;
	// The title sits above the list; together they are centred in the free height, the list a little lower.
	const float ListCenterY = std::min(DesignHeight / 2 - Style().ListCenterOffset, (DesignHeight - TitleHeight - Style().TitleGap - ListHeight) / 2 + ListHeight / 2);
	const cocos2d::Vec2 ListCenter(DesignWidth / 2, ListCenterY);
	List->LayoutVertically(ListCenter, Spacing);
	Title->setPosition(DesignWidth / 2, std::min(DesignHeight - Style().TitleTopOffset, ListCenterY + ListHeight / 2 + Style().TitleGap + TitleHeight / 2));

	if ( Where == Place::Level )
	{
		const float Top = Title->getPositionY() + TitleHeight / 2 + Style().PanelPadding;
		const float Bottom = ListCenter.y - ListHeight / 2 - Style().PanelPadding;
		const float HalfWidth = Style().ButtonWidth * Scale / 2 + Style().PanelPadding;
		auto Panel = cocos2d::DrawNode::create();
		Panel->drawSolidRect(cocos2d::Vec2(DesignWidth / 2 - HalfWidth, Bottom), cocos2d::Vec2(DesignWidth / 2 + HalfWidth, Top), widgets::colors::Panel());
		Panel->drawRect(cocos2d::Vec2(DesignWidth / 2 - HalfWidth, Bottom), cocos2d::Vec2(DesignWidth / 2 + HalfWidth, Top), widgets::colors::PanelBorder());
		addChild(Panel, -1);
	}
}

void SettingsPanel::AddEntry(const std::function<std::string()>& InCaption, const std::function<void()>& InAction, const bool bInEnabled)
{
	auto NewButton = widgets::Button::create(InCaption(), widgets::ScaleUi(cocos2d::Size(Style().ButtonWidth, Style().ButtonHeight)), InAction);
	NewButton->SetEnabled(bInEnabled);
	addChild(NewButton, 1);
	List->AddButton(NewButton);
	Entries.push_back({ NewButton, InCaption });
}

void SettingsPanel::AddSetting(const std::function<std::string()>& InCaption, const ProfileChange& InChange, const bool bInWindowChange, const bool bInEnabled)
{
	AddEntry(InCaption, [this, InChange, bInWindowChange]() { Apply(InChange, bInWindowChange); }, bInEnabled);
}

void SettingsPanel::Apply(const ProfileChange& InChange, const bool bInWindowChange)
{
	auto& Profiles = GameContext::Get().GetManager<ProfileManager>();
	InChange(Profiles.Get());
	if ( bInWindowChange )
	{
		Profiles.ApplyWindowSettings();
	}
	else
	{
		Profiles.ApplySettings();
	}
	if ( Where == Place::Level )
	{
		ApplyFpsLimit(Profiles.Get().FpsLimit);
	}
	Profiles.Save();
	Refresh();
}

void SettingsPanel::Refresh()
{
	Title->setString(Text("SETTINGS_TITLE"));
	for ( const Entry& Current : Entries )
	{
		Current.Target->SetText(Current.Caption());
	}
}

bool SettingsPanel::IsTextureQualityEnabled() const
{
	return TextureButton && TextureButton->IsEnabled();
}

} // namespace tanks::view::cocos

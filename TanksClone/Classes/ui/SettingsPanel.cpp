#include "services/UiStyle.h"
#include "ui/SettingsPanel.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "model/WidgetLayout.h"
#include "services/AudioManager.h"
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

	std::string NextLanguage()
	{
		const StringsManager& Strings = GameContext::Get().GetManager<StringsManager>();
		return Strings.GetNextLanguageId(Strings.GetLanguageId());
	}

	std::string LanguageName()
	{
		const StringsManager& Strings = GameContext::Get().GetManager<StringsManager>();
		const LanguageDef* const Current = Strings.FindLanguage(Strings.GetLanguageId());
		return Current ? Current->Name : Strings.GetLanguageId();
	}

	std::string AudioDeviceLabel(const std::string& InDevice)
	{
		if ( InDevice.empty() )
		{
			return Text("AUDIO_DEVICE_DEFAULT");
		}
		constexpr std::string_view OpenAlPrefix = "OpenAL Soft on ";
		std::string Label = InDevice.rfind(OpenAlPrefix, 0) == 0 ? InDevice.substr(OpenAlPrefix.size()) : InDevice;
		const size_t MaxChars = static_cast<size_t>(std::max(Style().DeviceNameChars, 4));
		if ( Label.size() > MaxChars )
		{
			Label = Label.substr(0, MaxChars - 3) + "...";
		}
		return Label;
	}

	std::string NextAudioDevice(const std::string& InCurrent, const int InDirection)
	{
		std::vector<std::string> Options = GameContext::Get().GetManager<AudioManager>().GetDevices();
		Options.insert(Options.begin(), std::string());
		const auto Found = std::find(Options.begin(), Options.end(), InCurrent);
		const int Count = static_cast<int>(Options.size());
		const int Index = Found == Options.end() ? 0 : static_cast<int>(Found - Options.begin());
		return Options[static_cast<size_t>(((Index + InDirection) % Count + Count) % Count)];
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

	AddSetting([]() { return Text("SETTINGS_LANGUAGE") + ": " + LanguageName(); },
		[](PlayerProfile& InProfile) { InProfile.CurrentLanguage = NextLanguage(); });
	AddEntry([]() { return Text("SETTINGS_TEXT_SIZE") + ": " + Text(TextSizeName(Profile().CurrentTextSize)); }, [this, InOnReopen]()
	{
		Apply([](PlayerProfile& InProfile) { InProfile.CurrentTextSize = model::TextScales::Next(InProfile.CurrentTextSize); }, false);
		InOnReopen();
	});
	Entries.back().Target->SetAdjust([this, InOnReopen](int)
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
	AddSetting([]() { return Text("SETTINGS_MUSIC_VOLUME") + ": " + std::to_string(Profile().MusicVolume) + "%"; },
		[](PlayerProfile& InProfile) { InProfile.MusicVolume = InProfile.MusicVolume <= 0 ? MaxVolume : std::max(InProfile.MusicVolume - Style().VolumeStep, 0); });
	Entries.back().Target->SetAdjust([this](const int InDirection)
	{
		Apply([InDirection](PlayerProfile& InProfile)
		{
			InProfile.MusicVolume = std::clamp(InProfile.MusicVolume + InDirection * Style().VolumeStep, 0, MaxVolume);
		}, false);
	});
	AddSetting([]() { return Text("SETTINGS_AUDIO_DEVICE") + ": " + AudioDeviceLabel(Profile().AudioDevice); },
		[](PlayerProfile& InProfile) { InProfile.AudioDevice = NextAudioDevice(InProfile.AudioDevice, 1); });
	Entries.back().Target->SetAdjust([this](const int InDirection)
	{
		Apply([InDirection](PlayerProfile& InProfile) { InProfile.AudioDevice = NextAudioDevice(InProfile.AudioDevice, InDirection); }, false);
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
	const float TextScale = widgets::GetUiScale();
	const float Margin = Style().ScreenMargin;
	Title->setScale(1.0f);
	const float TitleFit = std::min(1.0f, (DesignWidth - 2 * Margin) / std::max(Title->getContentSize().width, 1.0f));
	Title->setScale(TitleFit);
	const float TitleHeight = Title->getContentSize().height * TitleFit;

	const float Count = static_cast<float>(Entries.size());
	const float FullHeight = Style().ButtonHeight * TextScale;
	const float FullSpacing = Style().ButtonSpacing * TextScale;
	const float FullListHeight = Count * (FullHeight + FullSpacing) - FullSpacing;
	const float Factor = model::widget_layout::FitFactor(DesignHeight - 2 * Margin, TitleHeight + Style().TitleGap, FullListHeight, Style().MinFitFactor);
	const float ButtonHeight = FullHeight * Factor;
	const float Spacing = FullSpacing * Factor;
	const float ListHeight = Count * (ButtonHeight + Spacing) - Spacing;
	for ( const Entry& Current : Entries )
	{
		Current.Target->SetSize(cocos2d::Size(Style().ButtonWidth * TextScale, ButtonHeight));
	}

	// The title sits above the list; together they are centred in the free height, the list a little lower.
	const float GroupHeight = TitleHeight + Style().TitleGap + ListHeight;
	float ListCenterY = std::min(DesignHeight / 2 - Style().ListCenterOffset, (DesignHeight - GroupHeight) / 2 + ListHeight / 2);
	const float TitleTop = ListCenterY + ListHeight / 2 + Style().TitleGap + TitleHeight;
	ListCenterY -= std::max(TitleTop - (DesignHeight - Margin), 0.0f);
	const cocos2d::Vec2 ListCenter(DesignWidth / 2, ListCenterY);
	List->LayoutVertically(ListCenter, Spacing);
	const float TitleY = ListCenterY + ListHeight / 2 + Style().TitleGap + TitleHeight / 2;
	Title->setPosition(DesignWidth / 2, TitleY);

	if ( Where == Place::Level )
	{
		if ( !PanelBackground )
		{
			PanelBackground = cocos2d::DrawNode::create();
			addChild(PanelBackground, -1);
		}
		const float Top = TitleY + TitleHeight / 2 + Style().PanelPadding;
		const float Bottom = ListCenter.y - ListHeight / 2 - Style().PanelPadding;
		const float HalfWidth = Style().ButtonWidth * TextScale / 2 + Style().PanelPadding;
		PanelBackground->clear();
		PanelBackground->drawSolidRect(cocos2d::Vec2(DesignWidth / 2 - HalfWidth, Bottom), cocos2d::Vec2(DesignWidth / 2 + HalfWidth, Top), widgets::colors::Panel());
		PanelBackground->drawRect(cocos2d::Vec2(DesignWidth / 2 - HalfWidth, Bottom), cocos2d::Vec2(DesignWidth / 2 + HalfWidth, Top), widgets::colors::PanelBorder());
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
	Entries.back().Target->SetAdjust([this, InChange, bInWindowChange](int) { Apply(InChange, bInWindowChange); });
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
	Layout();
}

bool SettingsPanel::IsTextureQualityEnabled() const
{
	return TextureButton && TextureButton->IsEnabled();
}

} // namespace tanks::view::cocos

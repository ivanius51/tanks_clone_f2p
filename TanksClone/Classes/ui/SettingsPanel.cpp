#include "services/UiStyle.h"
#include "ui/SettingsPanel.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/WidgetLayout.h"
#include "services/AudioManager.h"
#include "services/DifficultyManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "ui/LayoutWidget.h"
#include "ui/MenuBuilder.h"
#include "ui/WidgetFactory.h"
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

	Panel = WidgetFactory::Create(ids::widgets::SettingsPanel);
	if ( !Panel )
	{
		return false;
	}
	addChild(Panel);
	const viewmodel::MenuDef* const Menu = GameContext::Get().GetManager<UiStyle>().FindMenu(ids::menus::Settings);
	if ( !Menu )
	{
		return false;
	}
	Panel->SetText("title", Text("SETTINGS_TITLE"));
	List = widgets::ButtonList::create();
	List->SetBelongsToOverlay(Where == Place::Level);
	addChild(List);

	Bindings = MakeBindings(InOnBack, InOnReopen);
	for ( const MenuEntry& Current : ResolveMenuEntries(*Menu, Bindings, false) )
	{
		AddEntry(Current);
	}
	Layout();
	return true;
}

MenuBindings SettingsPanel::MakeBindings(const std::function<void()>& InOnBack, const std::function<void()>& InOnReopen)
{
	MenuBindings Result;
	Result.Conditions["is_screen"] = [this]() { return Where == Place::Screen; };
	Result.Values = {
		{ "language", []() { return LanguageName(); } },
		{ "text_size", []() { return Text(TextSizeName(Profile().CurrentTextSize)); } },
		{ "difficulty", []() { return Text(GameContext::Get().GetManager<DifficultyManager>().GetCurrent().NameKey.c_str()); } },
		{ "sound", []() { return Text(Profile().bSoundEnabled ? "ON" : "OFF"); } },
		{ "effects_volume", []() { return std::to_string(Profile().EffectsVolume) + "%"; } },
		{ "music_volume", []() { return std::to_string(Profile().MusicVolume) + "%"; } },
		{ "audio_device", []() { return AudioDeviceLabel(Profile().AudioDevice); } },
		{ "fps", []() { return Profile().FpsLimit > 0 ? std::to_string(Profile().FpsLimit) : Text("FPS_UNLIMITED"); } },
		{ "vsync", []() { return Text(Profile().bVerticalSync ? "ON" : "OFF"); } },
		{ "window_mode", []() { return Text(Profile().CurrentWindowMode == WindowMode::Borderless ? "WINDOW_MODE_BORDERLESS" : "WINDOW_MODE_WINDOWED"); } },
		{ "map_scale", []() { return Profile().MapScale > 0 ? std::to_string(Profile().MapScale) + "%" : Text("MAP_SCALE_AUTO"); } },
		{ "pixels", []() { return Text(Profile().bSmoothPixels ? "PIXELS_SMOOTH" : "PIXELS_SHARP"); } },
		{ "textures", []() { return Text(Profile().bHdTextures ? "TEXTURES_HD" : "TEXTURES_SD"); } }
	};
	const auto AddSetting = [this, &Result](const std::string& InId, const ProfileChange& InChange, const bool bInWindowChange)
	{
		Result.Actions[InId] = [this, InChange, bInWindowChange]() { Apply(InChange, bInWindowChange); };
		Result.Adjusts[InId] = [this, InChange, bInWindowChange](int) { Apply(InChange, bInWindowChange); };
	};
	AddSetting("language", [](PlayerProfile& InProfile) { InProfile.CurrentLanguage = NextLanguage(); }, false);
	AddSetting("difficulty", [](PlayerProfile&) { GameContext::Get().GetManager<DifficultyManager>().SelectNext(); }, false);
	AddSetting("sound", [](PlayerProfile& InProfile) { InProfile.bSoundEnabled = !InProfile.bSoundEnabled; }, false);
	AddSetting("fps", [](PlayerProfile& InProfile) { InProfile.FpsLimit = NextFpsOption(InProfile.FpsLimit); }, false);
	AddSetting("vsync", [](PlayerProfile& InProfile) { InProfile.bVerticalSync = !InProfile.bVerticalSync; }, false);
	AddSetting("window_mode", [](PlayerProfile& InProfile)
		{
			InProfile.CurrentWindowMode = InProfile.CurrentWindowMode == WindowMode::Borderless ? WindowMode::Windowed : WindowMode::Borderless;
		}, true);
	AddSetting("map_scale", [](PlayerProfile& InProfile) { InProfile.MapScale = NextMapScale(InProfile.MapScale); }, false);
	AddSetting("pixels", [](PlayerProfile& InProfile) { InProfile.bSmoothPixels = !InProfile.bSmoothPixels; }, false);
	AddSetting("textures", [](PlayerProfile& InProfile) { InProfile.bHdTextures = !InProfile.bHdTextures; }, false);

	const auto NextTextSize = [this, InOnReopen]()
	{
		Apply([](PlayerProfile& InProfile) { InProfile.CurrentTextSize = model::TextScales::Next(InProfile.CurrentTextSize); }, false);
		InOnReopen();
	};
	Result.Actions["text_size"] = NextTextSize;
	Result.Adjusts["text_size"] = [NextTextSize](int) { NextTextSize(); };

	Result.Actions["effects_volume"] = [this]()
	{
		Apply([](PlayerProfile& InProfile) { InProfile.EffectsVolume = InProfile.EffectsVolume <= 0 ? MaxVolume : std::max(InProfile.EffectsVolume - Style().VolumeStep, 0); }, false);
	};
	Result.Adjusts["effects_volume"] = [this](const int InDirection)
	{
		Apply([InDirection](PlayerProfile& InProfile) { InProfile.EffectsVolume = std::clamp(InProfile.EffectsVolume + InDirection * Style().VolumeStep, 0, MaxVolume); }, false);
	};
	Result.Actions["music_volume"] = [this]()
	{
		Apply([](PlayerProfile& InProfile) { InProfile.MusicVolume = InProfile.MusicVolume <= 0 ? MaxVolume : std::max(InProfile.MusicVolume - Style().VolumeStep, 0); }, false);
	};
	Result.Adjusts["music_volume"] = [this](const int InDirection)
	{
		Apply([InDirection](PlayerProfile& InProfile) { InProfile.MusicVolume = std::clamp(InProfile.MusicVolume + InDirection * Style().VolumeStep, 0, MaxVolume); }, false);
	};
	Result.Actions["audio_device"] = [this]()
	{
		Apply([](PlayerProfile& InProfile) { InProfile.AudioDevice = NextAudioDevice(InProfile.AudioDevice, 1); }, false);
	};
	Result.Adjusts["audio_device"] = [this](const int InDirection)
	{
		Apply([InDirection](PlayerProfile& InProfile) { InProfile.AudioDevice = NextAudioDevice(InProfile.AudioDevice, InDirection); }, false);
	};
	Result.Actions["back"] = InOnBack;
	return Result;
}

void SettingsPanel::Layout()
{
	const float TextScale = widgets::GetUiScale();
	const float Margin = Style().ScreenMargin;
	LayoutWidget* const Column = Panel->FindWidget("buttons");

	const float Count = static_cast<float>(Entries.size());
	const float FullHeight = Style().ButtonHeight * TextScale;
	const float FullSpacing = Style().ButtonSpacing * TextScale;
	const float FullListHeight = Count * (FullHeight + FullSpacing) - FullSpacing;
	const auto ApplyHeights = [this, Column](const float InHeight, const float InSpacing)
	{
		for ( const Entry& Current : Entries )
		{
			Current.Target->SetSize(cocos2d::Size(Style().ButtonWidth * widgets::GetUiScale(), InHeight));
		}
		Column->SetGap(InSpacing);
		Column->Relayout();
	};
	ApplyHeights(FullHeight, FullSpacing);
	const float FixedHeight = Panel->getContentSize().height - FullListHeight;
	const float Factor = model::widget_layout::FitFactor(DesignHeight - 2 * Margin, FixedHeight, FullListHeight, Style().MinFitFactor);
	ApplyHeights(FullHeight * Factor, FullSpacing * Factor);

	Panel->SetPartVisible("background", Where == Place::Level);
	const float Height = Panel->getContentSize().height;
	const float CenterY = std::clamp(DesignHeight / 2 - Style().ListCenterOffset, Margin + Height / 2, DesignHeight - Margin - Height / 2);
	Panel->setPosition(DesignWidth / 2, CenterY);
}

void SettingsPanel::AddEntry(const MenuEntry& InEntry)
{
	auto NewButton = widgets::Button::create(InEntry.Caption, widgets::ScaleUi(cocos2d::Size(Style().ButtonWidth, Style().ButtonHeight)), InEntry.Action);
	NewButton->SetEnabled(InEntry.bEnabled);
	if ( InEntry.Adjust )
	{
		NewButton->SetAdjust(InEntry.Adjust);
	}
	Panel->FindWidget("buttons")->AppendNode(NewButton);
	List->AddButton(NewButton);
	Entries.push_back({ NewButton, *InEntry.Item });
	if ( InEntry.Item->Id == "textures" )
	{
		TextureButton = NewButton;
	}
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
	Panel->SetText("title", Text("SETTINGS_TITLE"));
	for ( const Entry& Current : Entries )
	{
		Current.Target->SetText(MenuCaption(Current.Item, Bindings, false));
	}
	Layout();
}

bool SettingsPanel::IsTextureQualityEnabled() const
{
	return TextureButton && TextureButton->IsEnabled();
}

} // namespace tanks::view::cocos

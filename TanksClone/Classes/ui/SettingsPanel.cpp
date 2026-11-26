#include "services/UiStyle.h"
#include "ui/SettingsPanel.h"

#include <algorithm>
#include <charconv>
#include <memory>
#include <tuple>
#include "templates/TFunction.h"
#include <string>
#include <string_view>

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
#include "templates/TContainers.h"

namespace game::view::cocos
{

namespace
{
	const viewmodel::SettingsLayout& Style()
	{
		return GameContext::Get<UiStyle>().GetMenu().Settings;
	}

	viewmodel::MenuButtonMetrics Buttons()
	{
		return widgets::GetMenuButtonMetrics(GameContext::Get<UiStyle>().FindMenu(ids::menus::Settings), "settings");
	}


	std::string Text(const char* const InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
	}

	PlayerProfile& Profile()
	{
		return GameContext::Get<ProfileManager>().Get();
	}

	int ParseWhole(const std::string& InText)
	{
		int Number = 0;
		std::from_chars(InText.data(), InText.data() + InText.size(), Number);
		return Number;
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
		TArray<std::string> Options = GameContext::Get<AudioManager>().GetDevices();
		Options.Insert(Options.begin(), std::string());
		const auto Found = std::find(Options.begin(), Options.end(), InCurrent);
		const int Count = static_cast<int>(Options.Size());
		const int Index = Found == Options.end() ? 0 : static_cast<int>(Found - Options.begin());
		return Options[static_cast<size_t>(((Index + InDirection) % Count + Count) % Count)];
	}
}

SettingsPanel* SettingsPanel::create(const Place InPlace, const TFunction<void(), UiCallbackInlineSize>& InOnBack, const TFunction<void(), UiCallbackInlineSize>& InOnReopen)
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

bool SettingsPanel::init(const Place InPlace, const TFunction<void(), UiCallbackInlineSize>& InOnBack, const TFunction<void(), UiCallbackInlineSize>& InOnReopen)
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
	const viewmodel::MenuDef* const Menu = GameContext::Get<UiStyle>().FindMenu(ids::menus::Settings);
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

MenuBindings SettingsPanel::MakeBindings(const TFunction<void(), UiCallbackInlineSize>& InOnBack, const TFunction<void(), UiCallbackInlineSize>& InOnReopen)
{
	MenuBindings Result;
	Result.Conditions["is_screen"] = [this]() { return Where == Place::Screen; };
	Result.Conditions["sound_on"] = []() { return Profile().bSoundEnabled; };
	Result.Conditions["vsync_on"] = []() { return Profile().bVerticalSync; };
	Result.Values = {
		{ "sound", []() { return Text(Profile().bSoundEnabled ? "ON" : "OFF"); } },
		{ "effects_volume", []() { return std::to_string(Profile().EffectsVolume) + "%"; } },
		{ "music_volume", []() { return std::to_string(Profile().MusicVolume) + "%"; } },
		{ "audio_device", []() { return AudioDeviceLabel(Profile().AudioDevice); } },
		{ "vsync", []() { return Text(Profile().bVerticalSync ? "ON" : "OFF"); } },
	};
	const auto AddSetting = [this, &Result](const std::string& InId, const ProfileChange& InChange, const bool bInWindowChange)
	{
		const auto Change = MakeSharedValue<const ProfileChange>(InChange);
		Result.Actions[InId] = [this, Change, bInWindowChange]() { Apply(Change->Get(), bInWindowChange); };
		Result.Adjusts[InId] = [this, Change, bInWindowChange](int) { Apply(Change->Get(), bInWindowChange); };
	};
	AddSetting("sound", [](PlayerProfile& InProfile) { InProfile.bSoundEnabled = !InProfile.bSoundEnabled; }, false);
	AddSetting("vsync", [](PlayerProfile& InProfile) { InProfile.bVerticalSync = !InProfile.bVerticalSync; }, false);

	const auto AddIndex = [this, &Result](const std::string& InId, const TFunction<int(const PlayerProfile&)>& InGet,
		const TFunction<void(PlayerProfile&, int), UiCallbackInlineSize>& InSet, const bool bInWindowChange, const TFunction<void(), UiCallbackInlineSize>& InAfter)
	{
		using IndexHandlers = std::tuple<TFunction<int(const PlayerProfile&)>, TFunction<void(PlayerProfile&, int), UiCallbackInlineSize>, TFunction<void(), UiCallbackInlineSize>>;
		const auto Handlers = MakeSharedValue<const IndexHandlers>(InGet, InSet, InAfter);
		Result.Indexes[InId] = {
			[Handlers]() { return std::get<0>(Handlers->Get())(Profile()); },
			[this, Handlers, bInWindowChange](const int InIndex)
			{
				Apply([Handlers, InIndex](PlayerProfile& InProfile) { std::get<1>(Handlers->Get())(InProfile, InIndex); }, bInWindowChange);
				if ( const TFunction<void(), UiCallbackInlineSize>& After = std::get<2>(Handlers->Get()) )
				{
					After();
				}
			} };
	};
	AddIndex("window_mode", [](const PlayerProfile& InProfile) { return InProfile.CurrentWindowMode == EWindowMode::Borderless ? 1 : 0; },
		[](PlayerProfile& InProfile, const int InIndex) { InProfile.CurrentWindowMode = InIndex == 1 ? EWindowMode::Borderless : EWindowMode::Windowed; }, true, nullptr);
	AddIndex("pixels", [](const PlayerProfile& InProfile) { return InProfile.bSmoothPixels ? 1 : 0; },
		[](PlayerProfile& InProfile, const int InIndex) { InProfile.bSmoothPixels = InIndex == 1; }, false, nullptr);
	AddIndex("textures", [](const PlayerProfile& InProfile) { return InProfile.bHdTextures ? 1 : 0; },
		[](PlayerProfile& InProfile, const int InIndex) { InProfile.bHdTextures = InIndex == 1; }, false, nullptr);
	AddIndex("text_size", [](const PlayerProfile& InProfile) { return static_cast<int>(InProfile.CurrentTextSize); },
		[](PlayerProfile& InProfile, const int InIndex) { InProfile.CurrentTextSize = static_cast<model::ETextSize>(InIndex); }, false, InOnReopen);

	const auto AddChoice = [this, &Result](const std::string& InId, const TFunction<std::string()>& InGet,
		const TFunction<void(PlayerProfile&, const std::string&), UiCallbackInlineSize>& InSet)
	{
		const auto Set = MakeSharedValue<const TFunction<void(PlayerProfile&, const std::string&), UiCallbackInlineSize>>(InSet);
		Result.Choices[InId] = { InGet, [this, Set](const std::string& InOption) { Apply([Set, InOption](PlayerProfile& InProfile) { Set->Get()(InProfile, InOption); }, false); } };
	};
	AddChoice("language", []() { return std::string(GameContext::Get<StringsManager>().GetLanguageId().View()); },
		[](PlayerProfile& InProfile, const std::string& InOption) { InProfile.CurrentLanguage = InOption; });
	AddChoice("difficulty", []() { return GameContext::Get<DifficultyManager>().GetCurrent().NameKey; },
		[](PlayerProfile& InProfile, const std::string& InOption)
		{
			for ( const model::DifficultyDef& Mode : GameContext::Get<DifficultyManager>().GetBook().GetModes() )
			{
				if ( Mode.NameKey == InOption )
				{
					InProfile.Difficulty = Mode.Id;
				}
			}
		});
	AddChoice("fps", []() { return std::to_string(Profile().FpsLimit); },
		[](PlayerProfile& InProfile, const std::string& InOption) { InProfile.FpsLimit = ParseWhole(InOption); });
	AddChoice("map_scale", []() { return std::to_string(Profile().MapScale); },
		[](PlayerProfile& InProfile, const std::string& InOption) { InProfile.MapScale = ParseWhole(InOption); });

	Result.Numbers["effects_volume"] = { []() { return Profile().EffectsVolume; },
		[this](const int InValue) { Apply([InValue](PlayerProfile& InProfile) { InProfile.EffectsVolume = InValue; }, false); } };
	Result.Numbers["music_volume"] = { []() { return Profile().MusicVolume; },
		[this](const int InValue) { Apply([InValue](PlayerProfile& InProfile) { InProfile.MusicVolume = InValue; }, false); } };
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

	const float Count = static_cast<float>(Entries.Size());
	const float FullHeight = Buttons().Height * TextScale;
	const float FullSpacing = Buttons().Spacing * TextScale;
	const float FullListHeight = Count * (FullHeight + FullSpacing) - FullSpacing;
	const auto ApplyHeights = [this, Column](const float InHeight, const float InSpacing)
	{
		for ( const Entry& Current : Entries )
		{
			Current.Target->SetSize(cocos2d::Size(Buttons().Width * widgets::GetUiScale(), InHeight));
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

void SettingsPanel::ShowFill(widgets::Button& InOutButton, const viewmodel::MenuItemDef& InItem) const
{
	auto* const Number = Bindings.Numbers.Find(InItem.Id);
	if ( InItem.Kind != "slider" || !Number || InItem.Max <= InItem.Min )
	{
		return;
	}
	InOutButton.SetFillFraction((static_cast<float>(Number->Get()) - InItem.Min) / (InItem.Max - InItem.Min));
}

void SettingsPanel::AddEntry(const MenuEntry& InEntry)
{
	auto NewButton = widgets::Button::create(InEntry.Caption, widgets::ScaleUi(cocos2d::Size(Buttons().Width, Buttons().Height)), InEntry.Action);
	NewButton->SetEnabled(InEntry.bEnabled);
	NewButton->SetControl(widgets::Button::ControlFromKind(InEntry.Item->Kind), InEntry.bChecked);
	if ( InEntry.Adjust )
	{
		NewButton->SetAdjust(InEntry.Adjust);
	}
	if ( InEntry.Slide )
	{
		NewButton->SetSlide(InEntry.Slide);
	}
	ShowFill(*NewButton, *InEntry.Item);
	Panel->FindWidget("buttons")->AppendNode(NewButton);
	List->AddButton(NewButton);
	Entries.PushBack({ NewButton, *InEntry.Item });
	if ( InEntry.Item->Id == "textures" )
	{
		TextureButton = NewButton;
	}
}

void SettingsPanel::Apply(const ProfileChange& InChange, const bool bInWindowChange)
{
	auto& Profiles = GameContext::Get<ProfileManager>();
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
		Current.Target->SetControl(widgets::Button::ControlFromKind(Current.Item.Kind), MenuChecked(Current.Item, Bindings, false));
		ShowFill(*Current.Target, Current.Item);
	}
	Layout();
}

bool SettingsPanel::IsTextureQualityEnabled() const
{
	return TextureButton && TextureButton->IsEnabled();
}

} // namespace game::view::cocos

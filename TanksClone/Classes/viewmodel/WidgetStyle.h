#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"
#include "model/TextSize.h"

namespace tanks::viewmodel
{

/** The shared colors of every screen (configs/ui/widgets.json, "colors"). */
struct WidgetColors
{
	model::Color TextNormal{ 235, 235, 235 };
	model::Color TextDim{ 150, 150, 160 };
	model::Color Accent{ 255, 205, 70 };
	model::Color Good{ 110, 220, 110 };
	model::Color Bad{ 235, 90, 80 };
	model::Color Panel{ 20, 23, 31, 245 };
	model::Color PanelBorder{ 115, 128, 153 };
	model::Color ButtonColor{ 41, 48, 66 };
	model::Color ButtonPressed{ 26, 31, 43 };
	model::Color ButtonDisabled{ 28, 31, 38 };
	model::Color ButtonBorder{ 89, 102, 128 };
	model::Color Selection{ 255, 204, 64 };
	model::Color ScreenBackground{ 14, 17, 24 };
	model::Color GridLine{ 255, 255, 255, 10 };
	model::Color Dim{ 0, 0, 0, 170 };
	model::Color Outline{ 0, 0, 0 };

	// The color a widget file names ("accent", "button_border"); nullptr for an unknown name.
	const model::Color* Find(std::string_view InName) const;
};

/**
 * The look shared by all widgets (configs/ui/widgets.json): colors, text-size factors, buttons, overlays, toasts.
 * The initial values are the fallbacks for missing keys.
 */
struct WidgetStyle
{
	WidgetColors Colors;
	model::TextScales TextScales;
	std::string SystemFont = "Arial";
	// The pixel font is much wider than the plain one at the same size.
	float DecorativeFontScale = 0.55f;
	int LabelOutline = 2;

	float BorderWidth = 2.0f;
	float ButtonHeight = 52.0f;

	float OverlayWidth = 640.0f;
	float OverlayPadding = 28.0f;
	float OverlayButtonSpacing = 12.0f;
	float OverlayTitleFontSize = 40.0f;
	float OverlayMessageFontSize = 22.0f;
	float OverlayMessageGap = 20.0f;
	float OverlayButtonsGap = 24.0f;

	float ToastFontSize = 26.0f;
	float ToastShowTime = 2.0f;
	float ToastFadeTime = 0.4f;

	float SceneFadeTime = 0.25f;

	float HintsFontSize = 18.0f;
	float HintsMargin = 12.0f;

	// OutMissing gets the path of every missing key (its default is kept).
	static WidgetStyle FromConfig(const model::IConfigNode* InNode, std::vector<std::string>& OutMissing);
};

} // namespace tanks::viewmodel

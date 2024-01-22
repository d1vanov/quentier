/*
 * Copyright 2020-2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string_view>

namespace quentier::preferences::keys {

// Name of group within ApplicationSettings inside which preferences related
// to panel colors are stored
constexpr std::string_view panelColorsGroup = "PanelColors";

// Name of preference specifying the font color for Quentier's panels
constexpr std::string_view panelFontColor = "FontColor";

// Name of preference specifying the backround color for Quentier's panels
constexpr std::string_view panelBackgroundColor = "BackgroundColor";

// Name of preference specifying whether Quentier's panels should use
// a gradient of colors as a background
constexpr std::string_view panelUseBackgroundGradient = "UseBackgroundGradient";

// Name of preference specifying the base color for the gradient background
// of Quentier's panels
constexpr std::string_view panelBackgroundGradientBaseColor =
    "BackgroundGradientBaseColor";

// Name of preference specifying the number of lines for the gradient background
// of Quentier's panels
constexpr std::string_view panelBackgroundGradientLineCount =
    "BackgroundGradientLines";

// Name of preference specifying the size of an individual gradient background
// line. An array of such values is stored in preferences.
constexpr std::string_view panelBackgroundGradientLineSize =
    "BackgroundGradientLineValue";

// Name of preference specifying the color of an individual gradient background
// line. An array of such values is stored in preferences.
constexpr std::string_view panelBackgroundGradientLineColor =
    "BackgroundGradientLineColor";

} // namespace quentier::preferences::keys

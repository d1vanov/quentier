/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#include "NativeMenuBar.h"

#include <QCoreApplication>
#include <QtGlobal>

namespace quentier {
namespace preferences {
namespace defaults {

bool disableNativeMenuBar()
{
    if (QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuBar)) {
        return true;
    }

#if !defined(Q_OS_WIN) && !defined(Q_OS_WIN32) && !defined(Q_OS_MACOS) &&      \
    !defined(Q_OS_MAC)
    // It appears there's a bug somewhere between Qt and Unity which causes
    // the native menu bar to not work properly, see
    // https://bugreports.qt.io/browse/QTCREATORBUG-17519
    const char * currentDesktopEnvVar = "XDG_CURRENT_DESKTOP";

    const auto currentDesktop =
        QString::fromUtf8(qgetenv(currentDesktopEnvVar)).toLower();

    if (currentDesktop.contains(QStringLiteral("unity"))) {
        return true;
    }
#endif

    return false;
}

} // namespace defaults
} // namespace preferences
} // namespace quentier

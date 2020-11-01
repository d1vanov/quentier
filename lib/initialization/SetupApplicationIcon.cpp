/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include "SetupApplicationIcon.h"

#include <quentier/utility/QuentierApplication.h>

#include <QIcon>

namespace quentier {

void setupApplicationIcon(QuentierApplication & app)
{
    QIcon icon;

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_512.png"), QSize(512, 512));

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_256.png"), QSize(256, 256));

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_128.png"), QSize(128, 128));

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_64.png"), QSize(64, 64));

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_48.png"), QSize(48, 48));

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_32.png"), QSize(32, 32));

    icon.addFile(
        QStringLiteral(":/app_icons/quentier_icon_16.png"), QSize(16, 16));

    app.setWindowIcon(icon);
}

} // namespace quentier

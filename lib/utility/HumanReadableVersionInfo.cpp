/*
 * Copyright 2018-2020 Dmitry Ivanov
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

#include "HumanReadableVersionInfo.h"

#include <VersionInfo.h>

#include <quentier/utility/VersionInfo.h>

namespace quentier {

QString quentierVersion()
{
    return QStringLiteral("Quentier ") +
        QStringLiteral(QUENTIER_MAJOR_VERSION) + QStringLiteral(".") +
        QStringLiteral(QUENTIER_MINOR_VERSION) + QStringLiteral(".") +
        QStringLiteral(QUENTIER_PATCH_VERSION);
}

QString quentierBuildInfo()
{
    return QStringLiteral(QUENTIER_BUILD_INFO);
}

QString libquentierBuildTimeInfo()
{
    QString info = QStringLiteral(QUENTIER_LIBQUENTIER_BINARY_NAME) +
        QStringLiteral("; version ") +
        QString::number(LIB_QUENTIER_VERSION_MAJOR) + QStringLiteral(".") +
        QString::number(LIB_QUENTIER_VERSION_MINOR) + QStringLiteral(".") +
        QString::number(LIB_QUENTIER_VERSION_PATCH) +
        QStringLiteral(", build info: ") +
        QStringLiteral(LIB_QUENTIER_BUILD_INFO) +
        QStringLiteral(", built with Qt ") +
        quentier::libquentierBuiltWithQtVersion();

#if LIB_QUENTIER_USE_QT_WEB_ENGINE
    info += QStringLiteral("; uses QtWebEngine");
#endif

    return info;
}

QString libquentierRuntimeInfo()
{
    QString info = QStringLiteral("version ") +
        QString::number(quentier::libquentierVersionMajor()) +
        QStringLiteral(".") +
        QString::number(quentier::libquentierVersionMinor()) +
        QStringLiteral(".") +
        QString::number(quentier::libquentierVersionPatch()) +
        QStringLiteral(", build info: ") + quentier::libquentierBuildInfo() +
        QStringLiteral(", built with Qt ") +
        quentier::libquentierBuiltWithQtVersion();

    if (quentier::libquentierUsesQtWebEngine()) {
        info += QStringLiteral("; uses QtWebEngine");
    }

    return info;
}

} // namespace quentier

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

#include <qt5qevercloud/VersionInfo.h>

#include <QTextStream>

namespace quentier {

QString quentierVersion()
{
    QString version;
    QTextStream strm(&version);

    strm << "Quentier " << QUENTIER_MAJOR_VERSION << "."
         << QUENTIER_MINOR_VERSION << "." << QUENTIER_PATCH_VERSION;

    return version;
}

QString quentierBuildInfo()
{
    return QStringLiteral(QUENTIER_BUILD_INFO);
}

QString libquentierBuildTimeInfo()
{
    QString info;
    QTextStream strm(&info);

    strm << QUENTIER_LIBQUENTIER_BINARY_NAME << ", version "
         << LIB_QUENTIER_VERSION_MAJOR << "." << LIB_QUENTIER_VERSION_MINOR
         << "." << LIB_QUENTIER_VERSION_PATCH
         << ", build info: " << LIB_QUENTIER_BUILD_INFO;

#if LIB_QUENTIER_USE_QT_WEB_ENGINE
    strm << "; uses QtWebEngine";
#endif

    return info;
}

QString libquentierRuntimeInfo()
{
    QString info;
    QTextStream strm(&info);

    strm << "version " << quentier::libquentierVersionMajor() << "."
         << quentier::libquentierVersionMinor() << "."
         << quentier::libquentierVersionPatch()
         << ", build info: " << quentier::libquentierBuildInfo()
         << ", built with Qt " << quentier::libquentierBuiltWithQtVersion();

    if (quentier::libquentierUsesQtWebEngine()) {
        strm << "; uses QtWebEngine";
    }

    return info;
}

QString qevercloudBuildTimeInfo()
{
    QString info;
    QTextStream strm(&info);

    strm << QUENTIER_QEVERCLOUD_BINARY_NAME << "; version "
         << QEVERCLOUD_VERSION_MAJOR << "." << QEVERCLOUD_VERSION_MINOR << "."
         << QEVERCLOUD_VERSION_PATCH
         << ", build info: " << QEVERCLOUD_BUILD_INFO;

#if QEVERCLOUD_USE_QT_WEB_ENGINE
    strm << "; uses QtWebEngine";
#endif

#if QEVERCLOUD_USES_Q_NAMESPACE
    strm << "; built with Q_NAMESPACE and Q_ENUM_NS support for error codes";
#endif

    return info;
}

QString qevercloudRuntimeInfo()
{
    QString info;
    QTextStream strm(&info);

    strm << "version " << qevercloud::qevercloudVersionMajor() << "."
         << qevercloud::qevercloudVersionMinor() << "."
         << qevercloud::qevercloudVersionPatch()
         << ", build info: " << qevercloud::qevercloudBuildInfo()
         << ", built with Qt " << qevercloud::qevercloudBuiltWithQtVersion();

    if (qevercloud::qevercloudUsesQtWebEngine()) {
        strm << "; uses QtWebEngine";
    }

    if (qevercloud::qevercloudUsesQNamespace()) {
        strm << "; built with Q_NAMESPACE and Q_ENUM_NS support for error "
             << "codes";
    }

    return info;
}

} // namespace quentier

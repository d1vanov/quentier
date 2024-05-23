/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include "SetupTranslations.h"

#include <lib/preferences/keys/Translations.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/Compat.h>

#include <QDir>
#include <QFileInfoList>
#include <QLocale>
#include <QStringList>
#include <QTranslator>

#include <string_view>
#include <utility>

namespace quentier {

void loadTranslations(
    const QString & defaultSearchPath, const std::string_view searchPathKey,
    const QString & filter, QTranslator & translator)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::translationGroup);
    QString searchPath = appSettings.value(searchPathKey).toString();
    appSettings.endGroup();

    QFileInfo searchPathInfo{searchPath};
    if (!searchPathInfo.exists()) {
        QNDEBUG(
            "initialization",
            "The translations search path read from app settings doesn't exist "
                << "(" << searchPath << "), fallback to the default search "
                << "path (" << defaultSearchPath << ")");

        searchPathInfo.setFile(defaultSearchPath);
    }
    else if (!searchPathInfo.isDir()) {
        QNDEBUG(
            "initialization",
            "The translations search path read from app settings is not a "
                << "directory (" << searchPath << "), fallback to the default "
                << "search path (" << defaultSearchPath << ")");

        searchPathInfo.setFile(defaultSearchPath);
    }
    else if (!searchPathInfo.isReadable()) {
        QNDEBUG(
            "initialization",
            "The translations search path read from app settings is not "
                << "readable (" << searchPath << "), fallback to the default "
                << "search path (" << defaultSearchPath << ")");

        searchPathInfo.setFile(defaultSearchPath);
    }

    searchPath = searchPathInfo.absoluteFilePath();

    QStringList nameFilter;
    nameFilter << filter;

    QString systemLocaleName = QLocale::system().name();
    QNDEBUG("initialization", "System locale name: " << systemLocaleName);

    QDir searchDir{searchPath};
    const auto translationFileInfos = searchDir.entryInfoList(nameFilter);
    for (const auto & translationFileInfo: std::as_const(translationFileInfos))
    {
        const QString translationBaseName = translationFileInfo.baseName();

        auto underscoreIndex =
            translationBaseName.lastIndexOf(QStringLiteral("_"));

        if (underscoreIndex > 0) {
            underscoreIndex = translationBaseName.lastIndexOf(
                QStringLiteral("_"), underscoreIndex - 1);
        }

        if (underscoreIndex > 0) {
            const QString langCode =
                translationBaseName.mid(underscoreIndex + 1);

            if (langCode == systemLocaleName) {
                QNDEBUG(
                    "initialization",
                    "Loading the translation for system locale: " << langCode
                        << ", file " << translationBaseName);
                if (!translator.load(translationFileInfo.absoluteFilePath())) {
                    QNWARNING(
                        "initialization",
                        "Failed to load translation for system locale: "
                            << langCode << ", file " << translationBaseName);
                }
            }
            else {
                QNDEBUG(
                    "initialization",
                    "Skipping translation not matching the system locale: "
                        << langCode << ", file: " << translationBaseName);
            }
        }
        else {
            QNDEBUG(
                "initialization",
                "Failed to figure out the language and country code "
                    << "corresponding to the translation file "
                    << translationBaseName);
        }
    }
}

void setupTranslations(QuentierApplication & app)
{
    QNDEBUG("initialization", "setupTranslations");

    QString defaultLibquentierTranslationsSearchPath =
        app.applicationDirPath() +
#ifdef Q_OS_MAC
        QStringLiteral("/../Resources/translations/libquentier");
#else
        QStringLiteral("/../share/libquentier/translations");
#endif

    QNDEBUG(
        "initialization",
        "Default libquentier translations search path: "
            << defaultLibquentierTranslationsSearchPath);

    const QString defaultQuentierTranslationsSearchPath = app.applicationDirPath() +
#ifdef Q_OS_MAC
        QStringLiteral("/../Resources/translations/quentier");
#else
        QStringLiteral("/../share/quentier/translations");
#endif

    QNDEBUG(
        "initialization",
        "Default quentier translations search path: "
            << defaultLibquentierTranslationsSearchPath);

    auto * translator = new QTranslator;

    QNDEBUG("initialization", "Loading translations for libquentier");

    loadTranslations(
        defaultLibquentierTranslationsSearchPath,
        preferences::keys::libquentierTranslationsSearchPath,
        QStringLiteral("libquentier_*.qm"), *translator);

    QNDEBUG("initialization", "Loading translations for quentier");

    loadTranslations(
        defaultQuentierTranslationsSearchPath,
        preferences::keys::quentierTranslationsSearchPath,
        QStringLiteral("quentier_*.qm"), *translator);

    app.installTranslator(translator);
}

} // namespace quentier

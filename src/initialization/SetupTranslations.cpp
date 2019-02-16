/*
 * Copyright 2017-2019 Dmitry Ivanov
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
#include "SettingsNames.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QTranslator>
#include <QFileInfoList>
#include <QStringList>
#include <QDir>
#include <QLocale>

namespace quentier {

void loadTranslations(const QString & defaultSearchPath,
                      const QString & searchPathSettingName,
                      const QString & filter, QTranslator & translator)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(TRANSLATION_SETTINGS_GROUP_NAME);
    QString searchPath = appSettings.value(searchPathSettingName).toString();
    appSettings.endGroup();

    QFileInfo searchPathInfo(searchPath);
    if (!searchPathInfo.exists())
    {
        QNDEBUG(QStringLiteral("The translations search path read from app ")
                << QStringLiteral("settings doesn't exist (") << searchPath
                << QStringLiteral("), fallback to the default search path (")
                << defaultSearchPath << QStringLiteral(")"));
        searchPathInfo.setFile(defaultSearchPath);
    }
    else if (!searchPathInfo.isDir())
    {
        QNDEBUG(QStringLiteral("The translations search path read from app ")
                << QStringLiteral("settings is not a directory (") << searchPath
                << QStringLiteral("), fallback to the default search path (")
                << defaultSearchPath << QStringLiteral(")"));
        searchPathInfo.setFile(defaultSearchPath);
    }
    else if (!searchPathInfo.isReadable())
    {
        QNDEBUG(QStringLiteral("The translations search path read from app ")
                << QStringLiteral("settings is not readable (") << searchPath
                << QStringLiteral("), fallback to the default search path (")
                << defaultSearchPath << QStringLiteral(")"));
        searchPathInfo.setFile(defaultSearchPath);
    }

    searchPath = searchPathInfo.absoluteFilePath();

    QStringList nameFilter;
    nameFilter << filter;

    QString systemLocaleName = QLocale::system().name();
    QNDEBUG(QStringLiteral("System locale name: ") << systemLocaleName);

    QDir searchDir(searchPath);
    QFileInfoList translationFileInfos = searchDir.entryInfoList(nameFilter);
    for(auto it = translationFileInfos.constBegin(),
        end = translationFileInfos.constEnd(); it != end; ++it)
    {
        const QFileInfo & translationFileInfo = *it;
        QString translationBaseName = translationFileInfo.baseName();
        int underscoreIndex = translationBaseName.lastIndexOf(QStringLiteral("_"));
        if (underscoreIndex > 0) {
            underscoreIndex = translationBaseName.lastIndexOf(QStringLiteral("_"),
                                                              underscoreIndex-1);
        }

        if (underscoreIndex > 0)
        {
            QString langCode = translationBaseName.mid(underscoreIndex + 1);
            if (langCode == systemLocaleName) {
                QNDEBUG(QStringLiteral("Loading the translation for system locale: ")
                        << langCode << QStringLiteral(", file ")
                        << translationBaseName);
                translator.load(translationFileInfo.absoluteFilePath());
            }
            else {
                QNDEBUG(QStringLiteral("Skipping translation not matching ")
                        << QStringLiteral("the system locale: ") << langCode
                        << QStringLiteral(", file: ") << translationBaseName);
            }
        }
        else
        {
            QNDEBUG(QStringLiteral("Failed to figure out the language and country ")
                    << QStringLiteral("code corresponding to the translation file ")
                    << translationBaseName);
        }
    }
}

void setupTranslations(QuentierApplication & app)
{
    QNDEBUG(QStringLiteral("setupTranslations"));

    QString defaultLibquentierTranslationsSearchPath =
        app.applicationDirPath() +
#ifdef Q_OS_MAC
        QStringLiteral("/../Resources/translations/libquentier");
#else
        QStringLiteral("/../share/libquentier/translations");
#endif
    QNDEBUG(QStringLiteral("Default libquentier translations search path: ")
            << defaultLibquentierTranslationsSearchPath);

    QString defaultQuentierTranslationsSearchPath =
        app.applicationDirPath() +
#ifdef Q_OS_MAC
        QStringLiteral("/../Resources/translations/quentier");
#else
        QStringLiteral("/../share/quentier/translations");
#endif
    QNDEBUG(QStringLiteral("Default quentier translations search path: ")
            << defaultLibquentierTranslationsSearchPath);

    QTranslator * pTranslator = new QTranslator;

    QNDEBUG(QStringLiteral("Loading translations for libquentier"));
    loadTranslations(defaultLibquentierTranslationsSearchPath,
                     LIBQUENTIER_TRANSLATIONS_SEARCH_PATH,
                     QStringLiteral("libquentier_*.qm"), *pTranslator);

    QNDEBUG(QStringLiteral("Loading translations for quentier"));
    loadTranslations(defaultQuentierTranslationsSearchPath,
                     QUENTIER_TRANSLATIONS_SEARCH_PATH,
                     QStringLiteral("quentier_*.qm"), *pTranslator);

    app.installTranslator(pTranslator);
}

} // namespace quentier

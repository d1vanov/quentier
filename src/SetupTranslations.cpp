/*
 * Copyright 2017 Dmitry Ivanov
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

namespace quentier {

void loadTranslations(const QString & defaultSearchPath, const QString & searchPathSettingName, const QString & filter,
                      QTranslator & translator)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(TRANSLATION_SETTINGS_GROUP_NAME);
    QString searchPath = appSettings.value(searchPathSettingName).toString();
    appSettings.endGroup();

    QFileInfo searchPathInfo(searchPath);
    if (!searchPathInfo.exists()) {
        QNDEBUG(QStringLiteral("The translations search path read from app settings doesn't exist (")
                << searchPath << QStringLiteral("), fallback to the default search path (")
                << defaultSearchPath << QStringLiteral(")"));
        searchPathInfo.setFile(defaultSearchPath);
    }
    else if (!searchPathInfo.isDir()) {
        QNDEBUG(QStringLiteral("The translations search path read from app settings is not a directory (")
                << searchPath << QStringLiteral("), fallback to the default search path (")
                << defaultSearchPath << QStringLiteral(")"));
        searchPathInfo.setFile(defaultSearchPath);
    }
    else if (!searchPathInfo.isReadable()) {
        QNDEBUG(QStringLiteral("The translations search path read from app settings is not readable (")
                << searchPath << QStringLiteral("), fallback to the default search path (")
                << defaultSearchPath << QStringLiteral(")"));
        searchPathInfo.setFile(defaultSearchPath);
    }

    searchPath = searchPathInfo.absoluteFilePath();

    QStringList nameFilter;
    nameFilter << filter;

    QDir searchDir(searchPath);
    QFileInfoList translationFileInfos = searchDir.entryInfoList(nameFilter);
    for(auto it = translationFileInfos.constBegin(), end = translationFileInfos.constEnd(); it != end; ++it) {
        const QFileInfo & translationFileInfo = *it;
        translator.load(translationFileInfo.absoluteFilePath());
    }
}

void setupTranslations(QuentierApplication & app)
{
    QNDEBUG(QStringLiteral("setupTranslations"));

    QString defaultLibquentierTranslationsSearchPath = app.applicationDirPath() + QStringLiteral("/../share/libquentier/translations");
    QNDEBUG(QStringLiteral("Default libquentier translations search path: ") <<defaultLibquentierTranslationsSearchPath);

    QString defaultQuentierTranslationsSearchPath = app.applicationDirPath() + QStringLiteral("/../share/quentier/translations");
    QNDEBUG(QStringLiteral("Default quentier translations search path: ") <<defaultLibquentierTranslationsSearchPath);

    QTranslator * pTranslator = new QTranslator;

    QNDEBUG(QStringLiteral("Loading translations for libquentier"));
    loadTranslations(defaultLibquentierTranslationsSearchPath, LIBQUENTIER_TRANSLATIONS_SEARCH_PATH,
                     QStringLiteral("libquentier_*.qm"), *pTranslator);

    QNDEBUG(QStringLiteral("Loading translationg for quentier"));
    loadTranslations(defaultQuentierTranslationsSearchPath, QUENTIER_TRANSLATIONS_SEARCH_PATH,
                     QStringLiteral("quentier_*.qm"), *pTranslator);

    app.installTranslator(pTranslator);
}

} // namespace quentier

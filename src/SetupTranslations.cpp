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

void setupTranslations(QuentierApplication & app)
{
    QNDEBUG(QStringLiteral("setupTranslations"));

    QString defaultLibquentierTranslationsSearchPath = app.applicationDirPath() + QStringLiteral("/../share/libquentier/translations");
    QNDEBUG(QStringLiteral("Default libquentier translations search path: ") <<defaultLibquentierTranslationsSearchPath);

    ApplicationSettings appSettings;
    appSettings.beginGroup(TRANSLATION_SETTINGS_GROUP_NAME);
    QString libquentierTranslationsSearchPath = appSettings.value(LIBQUENTIER_TRANSLATIONS_SEARCH_PATH).toString();
    appSettings.endGroup();

    QFileInfo libquentierTranslationsSearchPathInfo(libquentierTranslationsSearchPath);
    if (!libquentierTranslationsSearchPathInfo.exists()) {
        QNDEBUG(QStringLiteral("Libquentier translations search path read from app settings doesn't exist (")
                << libquentierTranslationsSearchPath << QStringLiteral("), fallback to the default search path"));
        libquentierTranslationsSearchPathInfo.setFile(defaultLibquentierTranslationsSearchPath);
    }
    else if (!libquentierTranslationsSearchPathInfo.isDir()) {
        QNDEBUG(QStringLiteral("Libquentier translations search path read from app settings is not a directory (")
                << libquentierTranslationsSearchPath << QStringLiteral("), fallback to the default search path"));
        libquentierTranslationsSearchPathInfo.setFile(defaultLibquentierTranslationsSearchPath);
    }
    else if (!libquentierTranslationsSearchPathInfo.isReadable()) {
        QNDEBUG(QStringLiteral("Libquentier translations search path read from app settings is not readable (")
                << libquentierTranslationsSearchPath << QStringLiteral("), fallback to the default search path"));
        libquentierTranslationsSearchPathInfo.setFile(defaultLibquentierTranslationsSearchPath);
    }

    libquentierTranslationsSearchPath = libquentierTranslationsSearchPathInfo.absoluteFilePath();

    QStringList libquentierTranslationsNameFilter;
    libquentierTranslationsNameFilter << QStringLiteral("libquentier_*.qm");

    QTranslator * pTranslator = new QTranslator;

    QDir libquentierTranslationsSearchDir(libquentierTranslationsSearchPath);
    QFileInfoList libquentierTranslationFileInfos = libquentierTranslationsSearchDir.entryInfoList(libquentierTranslationsNameFilter);
    for(auto it = libquentierTranslationFileInfos.constBegin(), end = libquentierTranslationFileInfos.constEnd(); it != end; ++it) {
        const QFileInfo & libquentierTranslationFileInfo = *it;
        pTranslator->load(libquentierTranslationFileInfo.absoluteFilePath());
    }

    QString quentierTranslationsSearchPath = app.applicationDirPath() + QStringLiteral("/../share/quentier/translations");
    QStringList quentierTranslationsNameFilter;
    quentierTranslationsNameFilter << QStringLiteral("quentier_*.qm");

    QDir quentierTranslationsSearchDir(quentierTranslationsSearchPath);
    QFileInfoList quentierTranslationFileInfos = quentierTranslationsSearchDir.entryInfoList(quentierTranslationsNameFilter);
    for(auto it = quentierTranslationFileInfos.constBegin(), end = quentierTranslationFileInfos.constEnd(); it != end; ++it) {
        const QFileInfo & quentierTranslationFileInfo = *it;
        pTranslator->load(quentierTranslationFileInfo.absoluteFilePath());
    }

    app.installTranslator(pTranslator);
}

} // namespace quentier

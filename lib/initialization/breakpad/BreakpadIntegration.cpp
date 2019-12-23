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

#include "BreakpadIntegration.h"

#include <quentier/utility/Macros.h>
#include <quentier/logging/QuentierLogger.h>

#include <QString>
#include <QFileInfoList>
#include <QDir>

namespace quentier {

void setQtWebEngineFlags()
{
    const char * envVar = "QTWEBENGINE_CHROMIUM_FLAGS";
    const char * disableInProcessStackTraces = "--disable-in-process-stack-traces";

    QByteArray flags = qgetenv(envVar);
    if (!flags.contains(disableInProcessStackTraces)) {
        qputenv(envVar, flags + " " + disableInProcessStackTraces);
    }
}

void findCompressedSymbolsFiles(
    const QApplication & app,
    QString & quentierCompressedSymbolsFilePath,
    QString & libquentierCompressedSymbolsFilePath)
{
    QNDEBUG("findCompressedSymbolsFiles");

    quentierCompressedSymbolsFilePath.resize(0);
    libquentierCompressedSymbolsFilePath.resize(0);

    QString appFilePath = app.applicationFilePath();
    QFileInfo appFileInfo(appFilePath);

    QDir appDir(appFileInfo.absoluteDir());

    QFileInfoList fileInfosNearApp = appDir.entryInfoList(QDir::Files);
    for(auto it = fileInfosNearApp.constBegin(),
        end = fileInfosNearApp.constEnd(); it != end; ++it)
    {
        const QFileInfo & fileInfo = *it;
        QString fileName = fileInfo.fileName();
        if (!fileName.endsWith(QStringLiteral(".syms.compressed"))) {
            continue;
        }

        if (fileName.startsWith(QStringLiteral("lib")))
        {
            if (fileName.contains(QStringLiteral("quentier"))) {
                libquentierCompressedSymbolsFilePath = fileInfo.absoluteFilePath();
                QNDEBUG("Found libquentier's compressed symbols file: "
                        << libquentierCompressedSymbolsFilePath);
            }

            continue;
        }

        if (fileName.startsWith(QStringLiteral("quentier"))) {
            quentierCompressedSymbolsFilePath = fileInfo.absoluteFilePath();
            QNDEBUG("Found quentier's compressed symbols file: "
                    << quentierCompressedSymbolsFilePath);
            continue;
        }
    }
}

} // namespace quentier

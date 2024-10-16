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

#include "BreakpadIntegration.h"

#include <quentier/logging/QuentierLogger.h>

#include <QFileInfoList>
#include <QDir>
#include <QString>

#include <utility>

namespace quentier {

void findCompressedSymbolsFiles(
    const QApplication & app,
    QString & quentierCompressedSymbolsFilePath,
    QString & libquentierCompressedSymbolsFilePath)
{
    QNDEBUG("initialization", "findCompressedSymbolsFiles");

    quentierCompressedSymbolsFilePath.resize(0);
    libquentierCompressedSymbolsFilePath.resize(0);

    const QString appFilePath = app.applicationFilePath();
    const QFileInfo appFileInfo{appFilePath};

    const QDir appDir{appFileInfo.absoluteDir()};

    const auto fileInfosNearApp = appDir.entryInfoList(QDir::Files);
    for (const auto & fileInfo: std::as_const(fileInfosNearApp)) {
        const QString fileName = fileInfo.fileName();
        if (!fileName.endsWith(QStringLiteral(".syms.compressed"))) {
            continue;
        }

        if (fileName.startsWith(QStringLiteral("lib")))
        {
            if (fileName.contains(QStringLiteral("quentier")))
            {
                libquentierCompressedSymbolsFilePath =
                    fileInfo.absoluteFilePath();

                QNDEBUG("initialization", "Found libquentier's compressed "
                    << "symbols file: "
                    << libquentierCompressedSymbolsFilePath);
            }

            continue;
        }

        if (fileName.startsWith(QStringLiteral("quentier"))) {
            quentierCompressedSymbolsFilePath = fileInfo.absoluteFilePath();
            QNDEBUG("initialization", "Found quentier's compressed symbols "
                << "file: " << quentierCompressedSymbolsFilePath);
            continue;
        }
    }
}

} // namespace quentier

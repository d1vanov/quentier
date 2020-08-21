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

#include "LoadDependencies.h"

#include <quentier/utility/VersionInfo.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QPluginLoader>
#include <QStringList>

namespace quentier {

void loadDependencies()
{
#ifdef Q_OS_WIN
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append(QStringLiteral("."));
    paths.append(QStringLiteral("imageformats"));
    paths.append(QStringLiteral("platforms"));
    paths.append(QStringLiteral("sqldrivers"));
    QCoreApplication::setLibraryPaths(paths);

    // Need to load the SQL drivers manually, for some reason Qt doesn't wish
    // to load them on its own
    QDirIterator sqlDriversIter(QStringLiteral("sqldrivers"));
    while (sqlDriversIter.hasNext()) {
        QString fileName = sqlDriversIter.next();
        if ((fileName == QStringLiteral("sqldrivers/.")) ||
            (fileName == QStringLiteral("sqldrivers/..")))
        {
            continue;
        }

        QPluginLoader pluginLoader(fileName);
        if (!pluginLoader.load()) {
            qWarning() << "Failed to load plugin " << fileName;
        }
    }
#endif // Q_OS_WIN
}

} // namespace quentier

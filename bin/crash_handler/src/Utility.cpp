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

#include "Utility.h"

#include <QDir>
#include <QFile>
#include <QFileInfoList>

QString nativePathToUnixPath(const QString & path)
{
    QString result = path;

#ifdef Q_OS_WIN
    result.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    result = QDir::fromNativeSeparators(result);
#endif

    return result;
}

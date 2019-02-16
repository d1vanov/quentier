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

#include "Utility.h"
#include <QDir>
#include <QFileInfoList>
#include <QFile>

QString nativePathToUnixPath(const QString & path)
{
    QString result = path;

#ifdef Q_OS_WIN
    result.replace(QString::fromUtf8("\\\\"), QString::fromUtf8("\\"));
    result = QDir::fromNativeSeparators(result);
#endif

    return result;
}

bool removeDir(const QString & dirPath)
{
    bool res = true;
    QDir dir(dirPath);

    if (dir.exists())
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        res = dir.removeRecursively();
#else
        QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot |
                                                  QDir::System |
                                                  QDir::Hidden |
                                                  QDir::AllDirs |
                                                  QDir::Files,
                                                  QDir::DirsFirst);
        for(auto it = entries.constBegin(),
            end = entries.constEnd(); it != end; ++it)
        {
            QFileInfo info(*it);
            if (info.isDir()) {
                res = removeDir(info.absoluteFilePath());
            }
            else {
                res = QFile::remove(info.absoluteFilePath());
            }

            if (!res) {
                return res;
            }
        }
        res = QDir().rmdir(dirPath);
#endif
    }

    return res;
}

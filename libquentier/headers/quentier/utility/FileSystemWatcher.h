/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_UTILITY_FILE_SYSTEM_WATCHER_H
#define LIB_QUENTIER_UTILITY_FILE_SYSTEM_WATCHER_H

#include <quentier/utility/Macros.h>
#include <QObject>
#include <QStringList>

#define FILE_SYSTEM_WATCHER_DEFAULT_REMOVAL_TIMEOUT_MSEC (500)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FileSystemWatcherPrivate)

class FileSystemWatcher: public QObject
{
    Q_OBJECT
public:
    explicit FileSystemWatcher(const int removalTimeoutMSec = FILE_SYSTEM_WATCHER_DEFAULT_REMOVAL_TIMEOUT_MSEC,
                               QObject * parent = Q_NULLPTR);
    explicit FileSystemWatcher(const QStringList & paths, const int removalTimeoutMSec = FILE_SYSTEM_WATCHER_DEFAULT_REMOVAL_TIMEOUT_MSEC,
                               QObject * parent = Q_NULLPTR);

    virtual ~FileSystemWatcher();

    void addPath(const QString & path);
    void addPaths(const QStringList & paths);

    QStringList directories() const;
    QStringList files() const;

    void removePath(const QString & path);
    void removePaths(const QStringList & paths);

Q_SIGNALS:
    void directoryChanged(const QString & path);
    void directoryRemoved(const QString & path);

    void fileChanged(const QString & path);
    void fileRemoved(const QString & path);

private:
    Q_DISABLE_COPY(FileSystemWatcher)

private:
    FileSystemWatcherPrivate * d_ptr;
    Q_DECLARE_PRIVATE(FileSystemWatcher)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_FILE_SYSTEM_WATCHER_H

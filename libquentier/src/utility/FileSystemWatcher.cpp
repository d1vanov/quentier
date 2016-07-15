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

#include <quentier/utility/FileSystemWatcher.h>
#include "FileSystemWatcher_p.h"

namespace quentier {

FileSystemWatcher::FileSystemWatcher(const int removalTimeoutMSec, QObject * parent) :
    QObject(parent),
    d_ptr(new FileSystemWatcherPrivate(*this, removalTimeoutMSec))
{}

FileSystemWatcher::FileSystemWatcher(const QStringList & paths, const int removalTimeoutMSec,
                                     QObject * parent) :
    QObject(parent),
    d_ptr(new FileSystemWatcherPrivate(*this, paths, removalTimeoutMSec))
{}

FileSystemWatcher::~FileSystemWatcher()
{}

void FileSystemWatcher::addPath(const QString & path)
{
    Q_D(FileSystemWatcher);
    d->addPath(path);
}

void FileSystemWatcher::addPaths(const QStringList & paths)
{
    Q_D(FileSystemWatcher);
    d->addPaths(paths);
}

QStringList FileSystemWatcher::directories() const
{
    Q_D(const FileSystemWatcher);
    return d->directories();
}

QStringList FileSystemWatcher::files() const
{
    Q_D(const FileSystemWatcher);
    return d->files();
}

void FileSystemWatcher::removePath(const QString & path)
{
    Q_D(FileSystemWatcher);
    d->removePath(path);
}

void FileSystemWatcher::removePaths(const QStringList & paths)
{
    Q_D(FileSystemWatcher);
    d->removePaths(paths);
}

} // namespace quentier

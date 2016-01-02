#include <qute_note/utility/FileSystemWatcher.h>
#include "FileSystemWatcher_p.h"

namespace qute_note {

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

} // namespace qute_note

#ifndef LIB_QUENTIER_UTILITY_FILE_SYSTEM_WATCHER_H
#define LIB_QUENTIER_UTILITY_FILE_SYSTEM_WATCHER_H

#include <quentier/utility/Qt4Helper.h>
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
